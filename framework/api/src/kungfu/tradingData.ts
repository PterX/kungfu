import { Subject } from 'rxjs';
import { watcher, startWatcher } from './watcher';
import {
  setTimerPromiseTask,
  // statTime,
  // statTimeEnd,
} from '@kungfu-trader/kungfu-js-api/utils/commonUtils';
import {
  getOrderResolved,
  getTradeResolved,
} from '@kungfu-trader/kungfu-js-api/utils/tradingUtils';
import { UnfinishedOrderStatus } from '@kungfu-trader/kungfu-js-api/config/tradingConfig';
import { DynamicIndexedMap } from '@kungfu-trader/kungfu-js-api/utils/classUtils';

const { startWatcherSyncTask } = useWatcher();
export const tradingDataSubject = new Subject<{
  watcher: KungfuApi.Watcher;
  tradingDataObject: KungfuApi.TradingDataObject;
}>();

export const triggerStartStep = (stepInterval = 2000) => {
  startWatcher();
  startWatcherSyncTask(stepInterval, (watcher, tradingDataObject) => {
    tradingDataSubject.next({ watcher, tradingDataObject });
  });
};

type TradingDataList = {
  orderList: KungfuApi.Order[];
  orderStatList: KungfuApi.OrderStat[];
  tradeList: KungfuApi.Trade[];
};

type AfterSync = (
  watcher: KungfuApi.Watcher,
  tradingDataObject: KungfuApi.TradingDataObject,
) => void;

const DEFAULT_SPLIT_LENGTH = 100;

export function useWatcher() {
  let dataQueue: TradingDataList[] = [];
  let isProcessing = false; // 标记是否正在处理队列中的数据
  const tradingDataObject: KungfuApi.TradingDataObject = {
    order: {
      td: {},
      strategy: {},
    },
    trade: {
      td: {},
      strategy: {},
    },
    position: {
      order: {},
      trade: {},
    },
  };

  interface ProcessingData {
    type: string;
    category: string;
    key: number;
    instrumentId: string;
    orderUKey: string;
    orderResolved: KungfuApi.OrderResolved | KungfuApi.TradeResolved;
  }

  type OrderStatsMap = {
    order: Record<string, KungfuApi.OrderStat>;
    trade: Record<string, KungfuApi.OrderStat>;
  };

  function orderDataProcessing(data: ProcessingData) {
    const { category, key, orderUKey, orderResolved } = data;
    if (!tradingDataObject.order[category][key]) {
      tradingDataObject.order[category][key] = new DynamicIndexedMap<
        string,
        KungfuApi.OrderResolved
      >(50000);
    }

    const target = tradingDataObject.order[category][key];
    const isFinished = !UnfinishedOrderStatus.includes(
      (orderResolved as KungfuApi.OrderResolved).status,
    );

    if (target.hasKey(orderUKey)) {
      target.updateKeyWithValue(orderUKey, orderResolved, 'order', isFinished);
    } else {
      target.insertKeyWithValue(orderUKey, orderResolved, 'order', isFinished);
    }
  }

  function positionDataProcessing(data: ProcessingData) {
    const { type, instrumentId, orderUKey, orderResolved } = data;
    if (!tradingDataObject.position[type][instrumentId]) {
      tradingDataObject.position[type][instrumentId] = new DynamicIndexedMap<
        string,
        KungfuApi.OrderResolved
      >(500);
    }
    const target = tradingDataObject.position[type][instrumentId];
    const isFinished = !UnfinishedOrderStatus.includes(
      (orderResolved as KungfuApi.OrderResolved).status,
    );
    if (target.hasKey(orderUKey)) {
      target.updateKeyWithValue(
        orderUKey,
        orderResolved,
        'position',
        isFinished,
      );
    } else {
      target.insertKeyWithValue(
        orderUKey,
        orderResolved,
        'position',
        isFinished,
      );
    }
  }

  function tradeDataProcessing(data: ProcessingData) {
    const { category, key, orderUKey, orderResolved } = data;

    if (!tradingDataObject.trade[category][key]) {
      tradingDataObject.trade[category][key] = new DynamicIndexedMap<
        string,
        KungfuApi.OrderResolved
      >(50000);
    }

    const target = tradingDataObject.trade[category][key];
    if (target.hasKey(orderUKey)) {
      target.updateKeyWithValue(orderUKey, orderResolved, 'trade');
    } else {
      target.insertKeyWithValue(orderUKey, orderResolved, 'trade');
    }
  }

  const drainStatesBySync = async () => {
    if (watcher === null) return;

    console.time('sync');
    watcher.sync();
    console.timeEnd('sync');
    const orderStatList = Object.values(watcher.ledger.OrderStat);
    const orderList = Object.values(watcher.ledger.Order);
    const tradeList = Object.values(watcher.ledger.Trade);
    console.log(
      'orderListLength',
      orderList.length,
      'tradeListLength',
      tradeList.length,
      'orderStatListLength',
      orderStatList.length,
    );
    dataQueue.push({
      orderList: orderList,
      tradeList: tradeList,
      orderStatList: orderStatList,
    });
  };

  const startWatcherSyncTask = (interval = 1000, callBack?: AfterSync) => {
    if (watcher === null) return;

    setTimerPromiseTask(async () => {
      await drainStatesBySync();
      callBack && callBack(watcher as KungfuApi.Watcher, tradingDataObject);
      console.log('tradingDataObject', tradingDataObject);
      callBack && processQueue();
    }, interval);
  };

  async function processQueue() {
    if (isProcessing) {
      return;
    }

    isProcessing = true;
    try {
      while (dataQueue.length > 0) {
        const data = dataQueue.shift();
        if (
          data &&
          (data.orderList.length > 0 ||
            data.tradeList.length > 0 ||
            data.orderStatList.length > 0)
        ) {
          await dealTradingDataObjectOptimized(
            data.orderList,
            data.tradeList,
            data.orderStatList,
          );
        }
      }
      isProcessing = false;
    } catch (error) {
      console.error('An error occurred while processing the queue:', error);
    }
  }

  function extractOrderStats(
    orderStatList: KungfuApi.OrderStat[],
  ): OrderStatsMap {
    const orderStats = { order: {}, trade: {} };
    orderStatList.forEach((stat) => {
      orderStats.order[stat.uid_key] = stat;
      orderStats.trade[stat.uid_key] = stat;
    });
    return orderStats;
  }

  function processAsync(data: ProcessingData): Promise<void> {
    return new Promise((resolve) => {
      processData(data);
      resolve();
    });
  }

  async function processOrderList(
    orderList: KungfuApi.Order[],
    orderStats: OrderStatsMap,
  ) {
    await doSomethingWithDataSliced(
      orderList,
      async (slicedOrderList, _sliceIndex) => {
        console.time('dealOrderList');

        const tasks: Promise<void>[] = [];
        for (let i = 0; i < slicedOrderList.length; i++) {
          const order = slicedOrderList[i];
          if (!order || !watcher) continue;

          const {
            instrument_id,
            exchange_id,
            source,
            dest,
            uid_key: orderUKey,
          } = order;
          const orderResolved = getOrderResolved(
            watcher,
            order,
            orderStats.order[orderUKey] || null,
          );
          const instrumentId = `${exchange_id}_${instrument_id}`;

          delete orderStats.order[orderUKey];

          tasks.push(
            processAsync({
              type: 'order',
              category: '',
              key: source,
              instrumentId,
              orderUKey,
              orderResolved,
            }),
          );

          tasks.push(
            processAsync({
              type: 'order',
              category: 'strategy',
              key: dest,
              instrumentId: '',
              orderUKey,
              orderResolved,
            }),
          );

          tasks.push(
            processAsync({
              type: 'order',
              category: 'td',
              key: source,
              instrumentId: '',
              orderUKey,
              orderResolved,
            }),
          );
        }

        await Promise.all(tasks);
        console.timeEnd('dealOrderList');
      },
      DEFAULT_SPLIT_LENGTH,
    );
  }

  async function processTradeList(
    tradeList: KungfuApi.Trade[],
    orderStats: OrderStatsMap,
  ) {
    await doSomethingWithDataSliced(
      tradeList,
      async (slicedTradeList, _sliceIndex) => {
        console.time('dealTradeList');
        const tasks: Promise<void>[] = [];
        for (let i = 0; i < slicedTradeList.length; i++) {
          const trade = slicedTradeList[i];
          if (!trade || !watcher) continue;
          const { instrument_id, exchange_id, source, dest } = trade;
          const orderUKey = trade.order_id.toString(16).padStart(16, '0');
          const tradeResolved = getTradeResolved(
            watcher,
            trade,
            orderStats.trade[orderUKey] || null,
          );
          const instrumentId = `${exchange_id}_${instrument_id}`;
          delete orderStats.trade[orderUKey];

          tasks.push(
            processAsync({
              type: 'trade',
              category: 'td',
              key: source,
              instrumentId: '',
              orderUKey,
              orderResolved: tradeResolved,
            }),
          );

          tasks.push(
            processAsync({
              type: 'trade',
              category: 'strategy',
              key: dest,
              instrumentId: '',
              orderUKey,
              orderResolved: tradeResolved,
            }),
          );

          tasks.push(
            processAsync({
              type: 'trade',
              category: '',
              key: dest,
              instrumentId,
              orderUKey,
              orderResolved: tradeResolved,
            }),
          );
        }

        await Promise.all(tasks);
        console.timeEnd('dealTradeList');
      },
      DEFAULT_SPLIT_LENGTH,
    );
  }

  function updateRemainingOrderStats(orderStats: OrderStatsMap) {
    console.time('dealOrderStats');

    const orderStatOfOrderKeys = Object.keys(orderStats.order);
    const tdKeys = Object.keys(tradingDataObject.order.td);
    const strategyKeys = Object.keys(tradingDataObject.order.strategy);

    for (let i = 0; i < orderStatOfOrderKeys.length; i++) {
      const key = orderStatOfOrderKeys[i];
      const orderStat = orderStats.order[key];
      if (!orderStat) continue;

      for (let j = 0; j < tdKeys.length; j++) {
        const source = tdKeys[j];
        const indexMap = tradingDataObject.order.td[source];
        if (!indexMap) continue;

        const order = indexMap.getValueForKey(key);
        if (!order) continue;

        const orderResolved = getOrderResolved(
          watcher as KungfuApi.Watcher,
          order,
          orderStat,
        );
        const instrumentId = `${order.exchange_id}_${order.instrument_id}`;
        const isFinished = !UnfinishedOrderStatus.includes(
          orderResolved.status,
        );

        indexMap.updateKeyWithValue(key, orderResolved, 'order', isFinished);
        if (tradingDataObject.position.order[instrumentId]) {
          tradingDataObject.position.order[instrumentId].updateKeyWithValue(
            key,
            orderResolved,
            'position',
            isFinished,
          );
        }
      }

      for (let k = 0; k < strategyKeys.length; k++) {
        const dest = strategyKeys[k];
        const indexMap = tradingDataObject.order.strategy[dest];
        if (!indexMap) continue;

        const order = indexMap.getValueForKey(key);
        if (!order) continue;

        const orderResolved = getOrderResolved(
          watcher as KungfuApi.Watcher,
          order,
          orderStat,
        );
        const isFinished = !UnfinishedOrderStatus.includes(
          orderResolved.status,
        );

        indexMap.updateKeyWithValue(key, orderResolved, 'order', isFinished);
      }
    }

    console.timeEnd('dealOrderStats');
  }

  function updateRemainingTradeStats(orderStats: OrderStatsMap) {
    const orderStatOfTradeKeys = Object.keys(orderStats.trade);

    if (orderStatOfTradeKeys.length > 0) {
      console.time('dealTradeStats');

      for (let i = 0; i < orderStatOfTradeKeys.length; i++) {
        const key = orderStatOfTradeKeys[i];

        const tdKeys = Object.keys(tradingDataObject.trade.td);
        for (let j = 0; j < tdKeys.length; j++) {
          const source = tdKeys[j];
          const indexMap = tradingDataObject.trade.td[source];
          if (!indexMap) continue;

          const trade = indexMap.getValueForKey(key);
          if (!trade) continue;

          const tradeResolved = getTradeResolved(
            watcher as KungfuApi.Watcher,
            trade,
            orderStats.trade[key],
          );
          const instrumentId = `${trade.exchange_id}_${trade.instrument_id}`;
          indexMap.updateKeyWithValue(key, tradeResolved, 'trade');
          if (tradingDataObject.position.trade[instrumentId]) {
            tradingDataObject.position.trade[instrumentId].updateKeyWithValue(
              key,
              tradeResolved,
              'position',
            );
          }
        }

        const strategyKeys = Object.keys(tradingDataObject.trade.strategy);
        for (let k = 0; k < strategyKeys.length; k++) {
          const dest = strategyKeys[k];
          const indexMap = tradingDataObject.trade.strategy[dest];
          if (!indexMap) continue;

          const trade = indexMap.getValueForKey(key);
          if (!trade) continue;

          const tradeResolved = getTradeResolved(
            watcher as KungfuApi.Watcher,
            trade,
            orderStats.trade[key],
          );

          indexMap.updateKeyWithValue(key, tradeResolved, 'trade');
        }
      }

      console.timeEnd('dealTradeStats');
    }
  }

  async function dealTradingDataObjectOptimized(
    orderList: KungfuApi.Order[],
    tradeList: KungfuApi.Trade[],
    orderStatList: KungfuApi.OrderStat[],
  ) {
    const orderStats = extractOrderStats(orderStatList);

    await processOrderList(orderList, orderStats);
    await processTradeList(tradeList, orderStats);

    await updateRemainingOrderStats(orderStats);
    await updateRemainingTradeStats(orderStats);
  }
  function processData(data: ProcessingData) {
    const { type, instrumentId } = data;

    if (instrumentId) {
      positionDataProcessing(data);
    } else if (type === 'order') {
      orderDataProcessing(data);
    } else {
      tradeDataProcessing(data);
    }
  }

  //释放队列资源
  function releaseQueue() {
    dataQueue = [];
  }
  return {
    startWatcherSyncTask,
    releaseQueue,
  };
}

export const dataOperationBySliceInEventLoop = <T>(
  data: T[],
  doSomethingCallback: (
    dataItem: T,
    index: number,
    sliceIndex: number,
  ) => Promise<void>,
  sliceLength = 1000,
) => {
  return new Promise<void>((resolve, reject) => {
    let i = 0,
      sliceIndex = 0;
    const len = data.length;
    const bestEventLoopTask =
      typeof window !== 'undefined'
        ? window.requestAnimationFrame
        : setImmediate;
    const runner = () => {
      bestEventLoopTask(async () => {
        for (let j = 0; j < sliceLength && i < len; i++, j++) {
          try {
            await doSomethingCallback(data[i], i, sliceIndex);
          } catch (error) {
            reject(error);
          }
        }

        if (i === len) {
          resolve();
        } else {
          sliceIndex++;
          runner();
        }
      });
    };

    runner();
  });
};

export const doSomethingWithDataSliced = <T>(
  data: T[],
  doSomethingCallback: (dataSliced: T[], sliceIndex: number) => Promise<void>,
  sliceLength = 1000,
) => {
  if (data.length === 0) return Promise.resolve();
  const dataSliced: T[] = [];
  return new Promise<void>((resolve, reject) => {
    dataOperationBySliceInEventLoop(
      data,
      async (dataItem, index, sliceIndex) => {
        dataSliced.push(dataItem);

        if (dataSliced.length === sliceLength || index === data.length - 1) {
          try {
            await doSomethingCallback(dataSliced, sliceIndex);
            dataSliced.length = 0;
          } catch (error) {
            reject(error);
          }
        }

        if (index === data.length - 1) resolve();
      },
      sliceLength,
    );
  });
};

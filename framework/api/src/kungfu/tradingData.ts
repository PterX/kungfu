import { Subject } from 'rxjs';
import { watcher, startWatcher } from './watcher';
import {
  setTimerPromiseTask,
  doSomethingWithDataSliced,
} from '@kungfu-trader/kungfu-js-api/utils/commonUtils';
import {
  getOrderResolved,
  getTradeResolved,
} from '@kungfu-trader/kungfu-js-api/utils/tradingUtils';
import { UnfinishedOrderStatus } from '@kungfu-trader/kungfu-js-api/config/tradingConfig';

export class DynamicTradingDataIndexedMap<K extends string | number, V> {
  private keyValueMap: { [key in K]?: V };
  private commonKeyIndexMap: { [key in K]?: number };
  private unfinishedKeyIndexMap: { [key in K]?: number };
  private commonList: V[];
  private unfinishedList: V[];
  private commonListOffset = 1;
  private commonSliceCount = 0;
  private unfinishedListOffset = 1;
  private updateFinishedIndexList: number[] = [];
  private maxCommonListLength = 50000;

  constructor(maxLength = 500) {
    this.keyValueMap = {};
    this.commonKeyIndexMap = {};
    this.unfinishedKeyIndexMap = {};
    this.commonList = [];
    this.unfinishedList = [];
    this.maxCommonListLength = maxLength;
  }

  countSmallerNumbers(num) {
    const length = this.updateFinishedIndexList.length;
    if (length === 0) {
      return 0;
    }

    let left = 0;
    let right = length;
    while (left < right) {
      const mid = Math.floor((left + right) / 2);
      if (this.updateFinishedIndexList[mid] < num) {
        left = mid + 1;
      } else {
        right = mid;
      }
    }

    return left;
  }

  insertKeyWithValue(key: K, value: V, type: string, isFinished = true): void {
    if (this.maxCommonListLength <= this.commonList.length) {
      this.deleteLastCommonValue();
    }

    this.commonKeyIndexMap[key] = --this.commonListOffset; // 为新键分配当前偏移量作为索引 插入新元素后减少偏移量
    this.commonList.unshift(value);
    this.keyValueMap[key] = value;
    if ((type === 'order' || type === 'position') && !isFinished) {
      this.unfinishedKeyIndexMap[key] = --this.unfinishedListOffset;
      this.unfinishedList.unshift(value);
    }
  }
  updateKeyWithValue(key: K, value: V, type: string, isFinished = true): void {
    const correctIndex = this.getCommonListIndexForKey(key);
    if (correctIndex !== undefined) {
      this.keyValueMap[key] = value;
      this.commonList.splice(correctIndex, 1, value);
    } else {
      return;
    }

    if (type === 'trade') {
      return;
    }

    const unfinishedCorrectIndex = this.getUnfinishedListIndexForKey(key);
    if (unfinishedCorrectIndex !== undefined) {
      if (isFinished) {
        this.unfinishedList.splice(unfinishedCorrectIndex, 1);
        this.updateFinishedIndexList.push(this.unfinishedKeyIndexMap[key] || 0);
        delete this.unfinishedKeyIndexMap[key];
      } else {
        this.unfinishedList.splice(unfinishedCorrectIndex, 1, value);
      }
    }
  }

  deleteLastCommonValue(): void {
    this.commonList.pop();
    this.commonSliceCount++;
  }

  getCommonListIndexForKey(key: K): number | undefined {
    const index = this.commonKeyIndexMap[key];

    if (index === undefined) {
      return undefined;
    }
    if (Number(index) + this.commonSliceCount > 0) {
      delete this.commonKeyIndexMap[key];
      return undefined;
    }
    return Number(index) - this.commonListOffset;
  }

  getUnfinishedListIndexForKey(key: K): number | undefined {
    const index = this.unfinishedKeyIndexMap[key];
    if (index === undefined) {
      return;
    }
    return (
      Number(index) -
      this.unfinishedListOffset -
      this.countSmallerNumbers(index)
    );
  }

  hasKey(key: K): boolean {
    return (
      this.commonKeyIndexMap[key] !== undefined ||
      this.unfinishedKeyIndexMap[key] !== undefined
    );
  }

  getValueForKey(key: K): V | undefined {
    return this.keyValueMap[key];
  }

  getKeyIndexMap(): { [key in K]?: number } {
    return this.commonKeyIndexMap;
  }

  getCommonList(): V[] {
    return [...this.commonList];
  }

  getUnfinishedList(): V[] {
    return [...this.unfinishedList];
  }
}

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

type ProcessingData = {
  type: 'order' | 'trade' | 'position';
  category: string;
  key: number | string;
  orderUKey: string;
  orderResolved: KungfuApi.OrderResolved | KungfuApi.TradeResolved;
};

type OrderStatsMap = {
  order: Record<string, KungfuApi.OrderStat>;
  trade: Record<string, KungfuApi.OrderStat>;
};

type AfterSync = (
  watcher: KungfuApi.Watcher,
  tradingDataObject: KungfuApi.TradingDataObject,
) => void;

const DEFAULT_SPLIT_LENGTH = 20;
const DEFAULT_TRADING_DATA_LENGTH = 50000;
const DEFAULT_POSITION_DATA_LENGTH = 500;

export function useWatcher() {
  let dataQueue: TradingDataList[] = [];
  let isProcessing = false; // 标记是否正在处理队列中的数据
  const tradingDataObject: KungfuApi.TradingDataObject = {
    order: {
      td: {},
      strategy: {},
      position: {},
    },
    trade: {
      td: {},
      strategy: {},
      position: {},
    },
  };

  //添加或更新数据
  function tradingDataProcessing(
    data: ProcessingData,
    dataType: 'order' | 'position' | 'trade',
  ) {
    const { category, key, orderUKey, orderResolved } = data;

    let defaultLength = 0;
    if (dataType === 'order' || dataType === 'trade') {
      defaultLength = DEFAULT_TRADING_DATA_LENGTH;
    } else if (dataType === 'position') {
      defaultLength = DEFAULT_POSITION_DATA_LENGTH;
    }

    if (!tradingDataObject[dataType][category][key]) {
      tradingDataObject[dataType][category][key] =
        new DynamicTradingDataIndexedMap<string, KungfuApi.OrderResolved>(
          defaultLength,
        );
    }

    const target = tradingDataObject[dataType][category][key];
    const isFinished =
      dataType !== 'trade'
        ? !UnfinishedOrderStatus.includes(
            (orderResolved as KungfuApi.OrderResolved).status,
          )
        : true;

    if (target.hasKey(orderUKey)) {
      target.updateKeyWithValue(orderUKey, orderResolved, dataType, isFinished);
    } else {
      target.insertKeyWithValue(orderUKey, orderResolved, dataType, isFinished);
    }
  }

  //根据watcher同步数据,并将数据推入队列
  const drainStatesBySync = async () => {
    if (watcher === null) return;

    console.time('sync');
    watcher.sync();
    console.timeEnd('sync');
    const orderStatList = Object.values(watcher.ledger.OrderStat);
    const orderList = Object.values(watcher.ledger.Order);
    const tradeList = Object.values(watcher.ledger.Trade);
    console.log('orderStatList:', orderStatList.length);
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
      console.log('tradingDataObject:', tradingDataObject);
      callBack && processQueue();
    }, interval);
  };

  //处理队列中的数据
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

  //释放队列资源
  function releaseQueue() {
    dataQueue = [];
  }

  //获取orderStatsMap
  function extractOrderStats(
    orderStatList: KungfuApi.OrderStat[],
  ): OrderStatsMap {
    const orderStatsMap = { order: {}, trade: {} };
    orderStatList.forEach((stat) => {
      orderStatsMap.order[stat.uid_key] = stat;
      orderStatsMap.trade[stat.uid_key] = stat;
    });
    return orderStatsMap;
  }

  async function processOrderList(
    orderList: KungfuApi.Order[],
    orderStatsMap: OrderStatsMap,
  ) {
    await doSomethingWithDataSliced(
      orderList,
      async (slicedOrderList) => {
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
          const orderIndexMap = tradingDataObject.order.td[source];

          const OldOrderResolved = orderIndexMap
            ? orderIndexMap.getValueForKey(orderUKey) || null
            : null;
          const orderResolved = getOrderResolved(
            watcher,
            OldOrderResolved,
            order,
            orderStatsMap.order[orderUKey] || null,
          );
          const instrumentId = `${exchange_id}_${instrument_id}`;

          delete orderStatsMap.order[orderUKey];

          tasks.push(
            Promise.resolve(
              processData({
                type: 'order',
                category: 'td',
                key: source,
                orderUKey,
                orderResolved,
              }),
            ),
          );

          tasks.push(
            Promise.resolve(
              processData({
                type: 'order',
                category: 'strategy',
                key: dest,
                orderUKey,
                orderResolved,
              }),
            ),
          );

          tasks.push(
            Promise.resolve(
              processData({
                type: 'order',
                category: 'position',
                key: instrumentId,

                orderUKey,
                orderResolved,
              }),
            ),
          );
        }

        await Promise.all(tasks);
      },
      DEFAULT_SPLIT_LENGTH,
    );
  }

  async function processTradeList(
    tradeList: KungfuApi.Trade[],
    orderStatsMap: OrderStatsMap,
  ) {
    await doSomethingWithDataSliced(
      tradeList,
      async (slicedTradeList) => {
        const tasks: Promise<void>[] = [];
        for (let i = 0; i < slicedTradeList.length; i++) {
          const trade = slicedTradeList[i];
          if (!trade || !watcher) continue;
          const { instrument_id, exchange_id, source, dest } = trade;
          const orderUKey = trade.order_id.toString(16).padStart(16, '0');
          const indexMap = tradingDataObject.trade.td[source];
          const oldTradeResolved = indexMap
            ? indexMap.getValueForKey(orderUKey) || null
            : null;
          const tradeResolved = getTradeResolved(
            watcher,
            oldTradeResolved,
            trade,
            orderStatsMap.trade[orderUKey] || null,
          );
          const instrumentId = `${exchange_id}_${instrument_id}`;
          delete orderStatsMap.trade[orderUKey];

          tasks.push(
            Promise.resolve(
              processData({
                type: 'trade',
                category: 'td',
                key: source,
                orderUKey,
                orderResolved: tradeResolved,
              }),
            ),
          );

          tasks.push(
            Promise.resolve(
              processData({
                type: 'trade',
                category: 'strategy',
                key: dest,
                orderUKey,
                orderResolved: tradeResolved,
              }),
            ),
          );

          tasks.push(
            Promise.resolve(
              processData({
                type: 'trade',
                category: 'position',
                key: instrumentId,
                orderUKey,
                orderResolved: tradeResolved,
              }),
            ),
          );
        }

        await Promise.all(tasks);
      },
      DEFAULT_SPLIT_LENGTH,
    );
  }

  //匹配order更新orderStat
  function updateRemainingOrderStats(orderStatsMap: OrderStatsMap) {
    const orderStatOfOrderKeys = Object.keys(orderStatsMap.order);
    const tdKeys = Object.keys(tradingDataObject.order.td);
    const strategyKeys = Object.keys(tradingDataObject.order.strategy);

    for (let i = 0; i < orderStatOfOrderKeys.length; i++) {
      const key = orderStatOfOrderKeys[i];
      const orderStat = orderStatsMap.order[key];
      if (!orderStat || !orderStat.insert_time) continue;

      for (let j = 0; j < tdKeys.length; j++) {
        const source = tdKeys[j];
        const indexMap = tradingDataObject.order.td[source];
        if (!indexMap) continue;

        const order = indexMap.getValueForKey(key);
        if (!order) continue;

        const orderResolved = getOrderResolved(
          watcher as KungfuApi.Watcher,
          null,
          order,
          orderStat,
        );
        const instrumentId = `${order.exchange_id}_${order.instrument_id}`;
        const isFinished = !UnfinishedOrderStatus.includes(
          orderResolved.status,
        );

        indexMap.updateKeyWithValue(key, orderResolved, 'order', isFinished);
        if (tradingDataObject.order.position[instrumentId]) {
          tradingDataObject.order.position[instrumentId].updateKeyWithValue(
            key,
            orderResolved,
            'position',
            isFinished,
          );
        }
      }

      for (let k = 0; k < strategyKeys.length; k++) {
        const dest = strategyKeys[k];
        const orderStat = orderStatsMap.order[key];
        if (!orderStat || !orderStat.insert_time) continue;
        const indexMap = tradingDataObject.order.strategy[dest];
        if (!indexMap) continue;

        const order = indexMap.getValueForKey(key);
        if (!order) continue;

        const orderResolved = getOrderResolved(
          watcher as KungfuApi.Watcher,
          null,
          order,
          orderStat,
        );
        const isFinished = !UnfinishedOrderStatus.includes(
          orderResolved.status,
        );

        indexMap.updateKeyWithValue(key, orderResolved, 'order', isFinished);
      }
    }
  }

  //匹配trade更新orderStat
  function updateRemainingTradeStats(orderStatsMap: OrderStatsMap) {
    const orderStatOfTradeKeys = Object.keys(orderStatsMap.trade);

    if (orderStatOfTradeKeys.length > 0) {
      for (let i = 0; i < orderStatOfTradeKeys.length; i++) {
        const key = orderStatOfTradeKeys[i];
        const orderStat = orderStatsMap.trade[key];
        if (!orderStat || !orderStat.trade_time) continue;

        const tdKeys = Object.keys(tradingDataObject.trade.td);
        for (let j = 0; j < tdKeys.length; j++) {
          const source = tdKeys[j];
          const indexMap = tradingDataObject.trade.td[source];
          if (!indexMap) continue;

          const trade = indexMap.getValueForKey(key);
          if (!trade) continue;

          const tradeResolved = getTradeResolved(
            watcher as KungfuApi.Watcher,
            null,
            trade,
            orderStat,
          );
          const instrumentId = `${trade.exchange_id}_${trade.instrument_id}`;
          indexMap.updateKeyWithValue(key, tradeResolved, 'trade');
          if (tradingDataObject.trade.position[instrumentId]) {
            tradingDataObject.trade.position[instrumentId].updateKeyWithValue(
              key,
              tradeResolved,
              'position',
            );
          }
        }

        const strategyKeys = Object.keys(tradingDataObject.trade.strategy);
        for (let k = 0; k < strategyKeys.length; k++) {
          const dest = strategyKeys[k];
          const orderStat = orderStatsMap.trade[key];
          if (!orderStat || !orderStat.trade_time) continue;
          const indexMap = tradingDataObject.trade.strategy[dest];
          if (!indexMap) continue;

          const trade = indexMap.getValueForKey(key);
          if (!trade) continue;

          const tradeResolved = getTradeResolved(
            watcher as KungfuApi.Watcher,
            null,
            trade,
            orderStat,
          );

          indexMap.updateKeyWithValue(key, tradeResolved, 'trade');
        }
      }
    }
  }

  async function dealTradingDataObjectOptimized(
    orderList: KungfuApi.Order[],
    tradeList: KungfuApi.Trade[],
    orderStatList: KungfuApi.OrderStat[],
  ) {
    const orderStatsMap = extractOrderStats(orderStatList);

    await processOrderList(orderList, orderStatsMap);
    await processTradeList(tradeList, orderStatsMap);

    await updateRemainingOrderStats(orderStatsMap);
    await updateRemainingTradeStats(orderStatsMap);
  }

  function processData(data: ProcessingData) {
    const { type } = data;

    tradingDataProcessing(data, type);
  }

  return {
    startWatcherSyncTask,
    releaseQueue,
  };
}

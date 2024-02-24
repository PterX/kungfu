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
  update: boolean;
}>();

export const triggerStartStep = (stepInterval = 2000) => {
  startWatcher();
  startWatcherSyncTask(stepInterval, (watcher, tradingDataObject, update) => {
    tradingDataSubject.next({ watcher, tradingDataObject, update });
  });
};

type TradingDataList = {
  orderList: KungfuApi.Order[];
  orderStatList: KungfuApi.OrderStat[];
  tradeList: KungfuApi.Trade[];
};

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
    position: {},
  };

  const drainStatesBySync = async () => {
    if (watcher === null) return;

    console.time('sync');
    watcher.sync();
    console.timeEnd('sync');
    const orderStatList = Object.values(watcher.ledger.OrderStat);
    const orderList = Object.values(watcher.ledger.Order);
    const tradeList = Object.values(watcher.ledger.Trade);

    dataQueue.push({
      orderList: orderList,
      tradeList: tradeList,
      orderStatList: orderStatList,
    });
  }

  type AfterSync = (
      watcher: KungfuApi.Watcher,
      tradingDataObject: KungfuApi.TradingDataObject,
      update: boolean,
    ) => void; 

  const startWatcherSyncTask = (
    interval = 1000,
    callBack?: AfterSync
  ) => {
    if (watcher === null) return;

    setTimerPromiseTask(async () => {
      await drainStatesBySync();
      callBack && callBack(watcher as KungfuApi.Watcher, tradingDataObject, true);
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

  async function dealTradingDataObjectOptimized(
    orderList: KungfuApi.Order[],
    tradeList: KungfuApi.Trade[],
    orderStatList: KungfuApi.OrderStat[],
  ) {
    const orderStats = {
      order: {},
      trade: {},
    };
    orderStatList.forEach((stat) => {
      orderStats.order[stat.uid_key] = stat;
      orderStats.trade[stat.uid_key] = stat;
    });
    
    //处理 orderList
    await doSomethingWithDataSliced(
      orderList,
      async (slicedOrderList, _sliceIndex) => {
        for (const order of slicedOrderList) {
          if (!order) continue;
          if (!watcher) return;
          const {
            // instrument_id,
            // exchange_id,
            source,
            dest,
            uid_key: orderUKey,
          } = order;

          const orderResolved = getOrderResolved(
            watcher as KungfuApi.Watcher,
            order,
            // watcher.ledger.OrderStat[orderUKey] || null,
            orderStats.order[orderUKey] || null,
          );
          // const positionId = `${exchange_id}_${instrument_id}`;

          delete orderStats.order[orderUKey];
          await processData({
            type: 'order',
            category: 'td',
            key: source,
            orderUKey,
            orderResolved,
          });
          // await processData({
          //   type: 'order',
          //   category: 'td',
          //   key: source,
          //   instrumentId: positionId,
          //   orderUKey,
          //   orderResolved,
          // });
          await processData({
            type: 'order',
            category: 'strategy',
            key: dest,
            orderUKey,
            orderResolved,
          });
        }
      },

      DEFAULT_SPLIT_LENGTH,
    );

    //处理 tradeList
    await doSomethingWithDataSliced(
      tradeList,
      async (slicedTradeList, _sliceIndex) => {
        console.time('doSomethingWithDataSliced');
        for (const trade of slicedTradeList) {
          if (!trade) continue;
          if (!watcher) return;
          const { source, dest } = trade;
          const orderUKey = trade.order_id.toString(16).padStart(16, '0');
          const tradeResolved = getTradeResolved(
            watcher as KungfuApi.Watcher,
            trade,
            orderStats.trade[orderUKey] || null,
          );
          delete orderStats.trade[orderUKey];
          await processData({
            type: 'trade',
            category: 'td',
            key: source,
            orderUKey,
            orderResolved: tradeResolved,
          });

          await processData({
            type: 'trade',
            category: 'strategy',
            key: dest,
            orderUKey,
            orderResolved: tradeResolved,
          });
        }
        console.timeEnd('doSomethingWithDataSliced');
      },
      DEFAULT_SPLIT_LENGTH,
    );

    // 处理剩余的 orderStats
    const orderStatOfOrderKeys = Object.keys(orderStats.order);
    const orderStatOfTradeKeys = Object.keys(orderStats.trade);
    if (orderStatOfOrderKeys.length > 0) {
      console.time('dealOrderStats');
      orderStatOfOrderKeys.forEach((key) => {
        Object.keys(tradingDataObject.order.td).forEach((source) => {
          if (tradingDataObject.order.td[source].indexMap) {
            const order =
              tradingDataObject.order.td[source].indexMap.getValueForKey(key);

            if (!order) return;
            const orderResolved = getOrderResolved(
              watcher as KungfuApi.Watcher,
              order,
              orderStats.order[key],
            );
            // const positionId = `${order.exchange_id}_${order.instrument_id}`;
            const isFinished = !UnfinishedOrderStatus.includes(
              orderResolved.status,
            );
            tradingDataObject.order.td[source].indexMap.updateKeyWithValue(
              key,
              orderResolved,
              'order',
              isFinished,
            );
            // tradingDataObject.position[
            //   positionId
            // ].positionIndexMap.updateKeyWithValue(
            //   key,
            //   orderResolved,
            //   'position',
            // );
          }
        });

        Object.keys(tradingDataObject.order.strategy).forEach((dest) => {
          if (tradingDataObject.order.strategy[dest].indexMap) {
            const order =
              tradingDataObject.order.strategy[dest].indexMap.getValueForKey(
                key,
              );

            if (!order) return;
            const orderResolved = getOrderResolved(
              watcher as KungfuApi.Watcher,
              order,
              orderStats.order[key],
            );
            const isFinished = !UnfinishedOrderStatus.includes(
              orderResolved.status,
            );
            tradingDataObject.order.strategy[dest].indexMap.updateKeyWithValue(
              key,
              orderResolved,
              'order',
              isFinished,
            );
          }
        });
      });
      console.timeEnd('dealOrderStats');
    }
    if (orderStatOfTradeKeys.length > 0) {
      console.time('dealTradeStats');
      orderStatOfTradeKeys.forEach((key) => {
        Object.keys(tradingDataObject.trade.td).forEach((source) => {
          if (tradingDataObject.trade.td[source].indexMap) {
            const trade =
              tradingDataObject.trade.td[source].indexMap.getValueForKey(key);

            if (!trade) return;
            const tradeResolved = getTradeResolved(
              watcher as KungfuApi.Watcher,
              trade,
              orderStats.trade[key],
            );

            tradingDataObject.trade.td[source].indexMap.updateKeyWithValue(
              key,
              tradeResolved,
              'trade',
            );
          }
        });

        Object.keys(tradingDataObject.trade.strategy).forEach((dest) => {
          if (tradingDataObject.trade.strategy[dest].indexMap) {
            const trade =
              tradingDataObject.trade.strategy[dest].indexMap.getValueForKey(
                key,
              );

            if (!trade) return;
            const tradeResolved = getTradeResolved(
              watcher as KungfuApi.Watcher,
              trade,
              orderStats.trade[key],
            );

            tradingDataObject.trade.strategy[dest].indexMap.updateKeyWithValue(
              key,
              tradeResolved,
              'trade',
            );
          }
        });
      });
      console.timeEnd('dealTradeStats');
    }
  }

  function processData(data: {
    type: string;
    category?: string;
    key?: number;
    instrumentId?: string;
    orderUKey: string;
    orderResolved: KungfuApi.OrderResolved | KungfuApi.TradeResolved;
  }) {
    const { type, category, key, instrumentId, orderUKey, orderResolved } =
      data;
    if (instrumentId) {
      if (!tradingDataObject.position[instrumentId]) {
        tradingDataObject.position[instrumentId] = {
          positionIndexMap: new DynamicIndexedMap<
            string,
            KungfuApi.OrderResolved
          >(500),
        };
      }
      const target = tradingDataObject.position[instrumentId];
      if (target.positionIndexMap.hasKey(orderUKey)) {
        target.positionIndexMap.updateKeyWithValue(
          orderUKey,
          orderResolved,
          'position',
        );
      } else {
        target.positionIndexMap.insertKeyWithValue(
          orderUKey,
          orderResolved,
          'position',
        );
      }
    } else {
      if (!tradingDataObject[type][category][key]) {
        tradingDataObject[type][category][key] = {
          indexMap: new DynamicIndexedMap<string, KungfuApi.OrderResolved>(
            50000,
          ),
        };
      }

      const target = tradingDataObject[type][category][key];
      if (type === 'order') {
        const isFinished = !UnfinishedOrderStatus.includes(
          (orderResolved as KungfuApi.OrderResolved).status,
        );

        if (target.indexMap.hasKey(orderUKey)) {
          target.indexMap.updateKeyWithValue(
            orderUKey,
            orderResolved,
            'order',
            isFinished,
          );
        } else {
          target.indexMap.insertKeyWithValue(
            orderUKey,
            orderResolved,
            'order',
            isFinished,
          );
        }
      } else {
        if (target.indexMap.hasKey(orderUKey)) {
          target.indexMap.updateKeyWithValue(orderUKey, orderResolved, 'trade');
        } else {
          target.indexMap.insertKeyWithValue(orderUKey, orderResolved, 'trade');
        }
      }
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

import { Subject } from 'rxjs';
import { watcher, startWatcher } from './watcher';
import {
  setTimerPromiseTask,
  doSomethingWithDataSliced,
} from '@kungfu-trader/kungfu-js-api/utils/commonUtils';
import {
  getOrderResolved,
  getTradeResolved,
  DEFAULT_LIST_LENGTH,
} from '@kungfu-trader/kungfu-js-api/utils/tradingUtils';
import { UnfinishedOrderStatus } from '@kungfu-trader/kungfu-js-api/config/tradingConfig';
import BTree from 'sorted-btree';
export class DynamicTradingDataIndexedMap<V> {
  private tradingDataType: 'order' | 'trade';
  private commonList: V[];
  private unfinishedList: V[];
  private maxListLength = DEFAULT_LIST_LENGTH;
  private commonTree: BTree<unknown, V>;
  private unfinishedTree: BTree<unknown, V>;
  private sortStr1 = '';
  private sortStr2 = '';
  private commonMinKey: unknown;

  constructor(type: 'order' | 'trade', maxListLength = DEFAULT_LIST_LENGTH) {
    this.tradingDataType = type;
    this.commonList = [];
    this.unfinishedList = [];
    this.maxListLength = maxListLength;
    this.commonMinKey = null;

    if (this.tradingDataType === 'order') {
      this.sortStr1 = 'insert_time';
      this.sortStr2 = 'order_id';
    } else {
      this.sortStr1 = 'trade_time';
      this.sortStr2 = 'trade_id';
    }
    this.commonTree = new BTree(undefined, (a, b) => {
      if ((b as V)[this.sortStr1] > (a as V)[this.sortStr1]) return 1;
      else if ((b as V)[this.sortStr1] < (a as V)[this.sortStr1]) return -1;
      else {
        return (b as V)[this.sortStr2]
          .toString()
          .localeCompare((a as V)[this.sortStr2].toString());
      }
    });
    this.unfinishedTree = new BTree(undefined, (a, b) => {
      if ((b as V)[this.sortStr1] > (a as V)[this.sortStr1]) return 1;
      else if ((b as V)[this.sortStr1] < (a as V)[this.sortStr1]) return -1;
      else {
        return (b as V)[this.sortStr2]
          .toString()
          .localeCompare((a as V)[this.sortStr2].toString());
      }
    });
  }

  insertKeyWithValue(value: V, type: string, isFinished = true): void {
    if (
      !this.commonMinKey ||
      !(this.commonMinKey as Record<string, bigint>)[this.sortStr1] ||
      (this.commonMinKey as Record<string, bigint>)[this.sortStr1] <=
        value[this.sortStr1]
    ) {
      this.commonTree.set(
        {
          [this.sortStr1]: value[this.sortStr1],
          [this.sortStr2]: value[this.sortStr2],
        },
        value,
        true,
      );
    }

    if (type === 'order' && !isFinished) {
      this.unfinishedTree.set(
        {
          [this.sortStr1]: value[this.sortStr1],
          [this.sortStr2]: value[this.sortStr2],
        },
        value,
        true,
      );
    }
  }

  updateKeyWithValue(value: V, type: string, isFinished = true): void {
    this.commonTree.changeIfPresent(
      {
        [this.sortStr1]: value[this.sortStr1],
        [this.sortStr2]: value[this.sortStr2],
      },
      value,
    );

    if (type === 'trade') {
      return;
    }

    if (isFinished) {
      this.unfinishedTree.delete({
        [this.sortStr1]: value[this.sortStr1],
        [this.sortStr2]: value[this.sortStr2],
      });
    } else {
      this.unfinishedTree.changeIfPresent(
        {
          [this.sortStr1]: value[this.sortStr1],
          [this.sortStr2]: value[this.sortStr2],
        },
        value,
      );
    }
  }

  getValue(key1: unknown, key2: unknown): V | undefined {
    return (
      this.commonTree.get({
        [this.sortStr1]: key1,
        [this.sortStr2]: key2,
      }) ||
      this.unfinishedTree.get({ [this.sortStr1]: key1, [this.sortStr2]: key2 })
    );
  }

  getCommonList(): V[] {
    return [...this.commonList];
  }

  getUnfinishedList(): V[] {
    return [...this.unfinishedList];
  }

  sortCommonList(): void {
    const keys = this.commonTree.keysArray();
    if (keys.length > this.maxListLength) {
      //删除多余的数据
      const redundantKeys = keys.slice(this.maxListLength);
      const lo = redundantKeys[0];
      const high = redundantKeys[redundantKeys.length - 1];
      this.commonTree.deleteRange(lo, high, true);
      this.commonMinKey = this.commonTree.maxKey(); //插入时间最小的Key
    }
    this.commonList = this.commonTree.valuesArray();
  }

  sortUnfinishedList(): void {
    this.unfinishedList = this.unfinishedTree.valuesArray();
  }

  getAllUnfinishedList(): V[] {
    return this.unfinishedTree.valuesArray();
  }

  getAllList(): V[] {
    //TODO: fullList
    return this.commonTree.valuesArray();
  }
}

const { startWatcherSyncTask } = useWatcher();
export const tradingDataSubject = new Subject<{
  watcher: KungfuApi.Watcher;
  tradingDataKeeper: KungfuApi.TradingDataKeeper;
}>();

export const triggerStartStep = (stepInterval = 2000) => {
  startWatcher();
  startWatcherSyncTask(stepInterval, (watcher, tradingDataKeeper, update) => {
    tradingDataKeeper.update = update;
    tradingDataSubject.next({ watcher, tradingDataKeeper });
  });
};

type TradingActivityData = {
  orderList: KungfuApi.Order[];
  orderStatList: KungfuApi.OrderStat[];
  tradeList: KungfuApi.Trade[];
};

type ProcessingData = {
  type: 'order' | 'trade';
  category: string;
  key: number | string;
  dataResolved: KungfuApi.OrderResolved | KungfuApi.TradeResolved;
};

type OrderStatsMap = {
  order: Record<string, KungfuApi.OrderStat>;
};

type AfterSync = (
  watcher: KungfuApi.Watcher,
  tradingDataKeeper: KungfuApi.TradingDataKeeper,
  update: boolean,
) => void;

const DEFAULT_SPLIT_LENGTH = 50;
const DEFAULT_TRADING_DATA_LENGTH = DEFAULT_LIST_LENGTH;

const bestEventLoopTask =
  typeof window !== 'undefined'
    ? globalThis.requestAnimationFrame
    : setImmediate;
export function useWatcher() {
  let update = true;
  let dataQueue: TradingActivityData[] = [];
  let isProcessing = false; // 标记是否正在处理队列中的数据
  const tradingDataKeeper: KungfuApi.TradingDataKeeper = {
    order: {
      td: {},
      strategy: {},
      list: function (): KungfuApi.OrderResolved[] {
        return Object.values(this.td).reduce((acc, cur) => {
          return acc.concat(cur.getAllList());
        }, [] as KungfuApi.OrderResolved[]);
      },
      filter: function (keyOrCallback, value): KungfuApi.OrderResolved[] {
        return typeof keyOrCallback === 'function'
          ? this.list().filter(keyOrCallback)
          : this.list().filter((order) => {
              return order[keyOrCallback] === value;
            });
      },
    },
    trade: {
      td: {},
      strategy: {},
      list: function (): KungfuApi.TradeResolved[] {
        return Object.values(this.td).reduce((acc, cur) => {
          return acc.concat(cur.getAllList());
        }, [] as KungfuApi.TradeResolved[]);
      },
      filter: function (keyOrCallback, value): KungfuApi.TradeResolved[] {
        return typeof keyOrCallback === 'function'
          ? this.list().filter(keyOrCallback)
          : this.list().filter((trade) => {
              return trade[keyOrCallback] === value;
            });
      },
    },
    update: false,
    sortedForEach: async function (
      callback: (
        tradingData: KungfuApi.OrderResolved | KungfuApi.TradeResolved,
      ) => boolean,
      tradingDataType: 'order' | 'trade',
      tradingDataGroup: 'td' | 'strategy',
      listGetterType: 'common' | 'unfinished',
      groupFilterKeys?: number[],
    ): Promise<void> {
      const listGetter =
        listGetterType === 'common' ? 'getCommonList' : 'getUnfinishedList';
      let flag = true;

      const indexMapList = groupFilterKeys
        ? groupFilterKeys
            .map((item) => this[tradingDataType][tradingDataGroup][item])
            .filter((item) => item)
        : Object.values(this[tradingDataType][tradingDataGroup]);

      let fullList = [];
      indexMapList.forEach((item) => {
        const list = item[listGetter]();
        fullList = fullList.concat(list);
      });

      const compare = (a, b) => {
        const str = a.trade_id ? 'trade_time' : 'insert_time';
        return Number(b[str]) - Number(a[str]);
      };

      const sortedList = fullList.sort(compare);

      const processSlice = async (slice) => {
        for (const item of slice) {
          flag = callback(item);
          if (!flag) return;
        }
      };

      const processInChunks = async (list, chunkSize = 10000) => {
        for (let i = 0; i < list.length; i += chunkSize) {
          const slice = list.slice(i, i + chunkSize);
          await new Promise((resolve) =>
            bestEventLoopTask(() => resolve(processSlice(slice))),
          );
          if (!flag) break;
        }
      };

      await processInChunks(sortedList);
    },
  };

  globalThis.TradingDataKeeper = tradingDataKeeper;

  const sortDataMap = new Map<
    string,
    KungfuApi.KfDynamicTradingDataIndexedMap<
      KungfuApi.OrderResolved | KungfuApi.TradeResolved
    >
  >();
  const unMatchedOrderStatMap: Record<string, KungfuApi.OrderStat> = {};

  //添加或更新数据
  function tradingDataProcessing(data: ProcessingData) {
    const { type, category, key, dataResolved } = data;

    if (!tradingDataKeeper[type][category][key]) {
      tradingDataKeeper[type][category][key] =
        new DynamicTradingDataIndexedMap<KungfuApi.OrderResolved>(
          type,
          DEFAULT_TRADING_DATA_LENGTH,
        );
    }

    const target = tradingDataKeeper[type][category][key];
    sortDataMap.set(`${type}_${category}_${key}`, target);

    if (type === 'trade') {
      target.insertKeyWithValue(dataResolved, type);
    } else {
      const key1 = (dataResolved as KungfuApi.OrderResolved).insert_time;
      const key2 = dataResolved.order_id;
      const isFinished = !UnfinishedOrderStatus.includes(
        (dataResolved as KungfuApi.OrderResolved).status,
      );
      if (target.getValue(key1, key2)) {
        target.updateKeyWithValue(dataResolved, type, isFinished);
      } else {
        target.insertKeyWithValue(dataResolved, type, isFinished);
      }
    }
  }

  //根据watcher同步数据,并将数据推入队列
  const drainStatesBySync = () => {
    if (watcher === null) return;
    watcher.sync();
    const orderStatList = Object.values(watcher.ledger.OrderStat);
    const orderList = Object.values(watcher.ledger.Order);
    const tradeList = Object.values(watcher.ledger.Trade);

    dataQueue.push({
      orderList: orderList,
      tradeList: tradeList,
      orderStatList: orderStatList,
    });
  };

  const startWatcherSyncTask = (interval = 1000, callBack?: AfterSync) => {
    if (watcher === null) return;

    setTimerPromiseTask(async () => {
      drainStatesBySync();
      callBack &&
        callBack(watcher as KungfuApi.Watcher, tradingDataKeeper, update);
      callBack && processQueue();
    }, interval);
  };

  const compare = (a, b) => {
    const str = a.trade_id ? 'trade_time' : 'insert_time';
    return Number(b[str]) - Number(a[str]);
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
          sortDataMap.clear();
          await dealTradingDataOptimized(
            data.orderList,
            data.tradeList,
            data.orderStatList,
          );
          const sortDataMapValues = Array.from(sortDataMap.values());
          const sortPromises: Promise<void>[] = [];
          for (let i = 0, len = sortDataMapValues.length; i < len; i++) {
            sortPromises.push(
              Promise.resolve(sortDataMapValues[i].sortCommonList(compare)),
            );
            sortPromises.push(
              Promise.resolve(sortDataMapValues[i].sortUnfinishedList(compare)),
            );
          }

          await Promise.all(sortPromises);
          update = true;
        } else {
          update = false;
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
    const orderStatsMap = { order: {} };
    orderStatList.forEach((stat) => {
      orderStatsMap.order[stat.uid_key] = stat;
    });
    return orderStatsMap;
  }

  function processOrderList(
    orderList: KungfuApi.Order[],
    orderStatsMap: OrderStatsMap,
  ) {
    return new Promise<void>((resolve) => {
      const handler = doSomethingWithDataSliced(
        orderList,
        (slicedOrderList) => {
          for (let i = 0; i < slicedOrderList.length; i++) {
            const order = slicedOrderList[i];
            if (!order || !watcher) continue;

            const {
              order_id,
              insert_time,
              source,
              dest,
              uid_key: orderUKey,
            } = order;
            const orderIndexMap = tradingDataKeeper.order.td[source];
            const OldOrderResolved = orderIndexMap
              ? orderIndexMap.getValue(insert_time, order_id) || null
              : null;
            const orderStatResolved = OldOrderResolved
              ? {
                  latency_system: OldOrderResolved.latency_system,
                  latency_network: OldOrderResolved.latency_network,
                  avg_price: OldOrderResolved.avg_price,
                }
              : null;
            const orderResolved = getOrderResolved(
              watcher,
              order,
              orderStatsMap.order[orderUKey] ||
                unMatchedOrderStatMap[orderUKey] ||
                null,
              orderStatResolved,
            );

            delete orderStatsMap.order[orderUKey];
            tradingDataProcessing({
              type: 'order',
              category: 'td',
              key: source,
              dataResolved: orderResolved,
            });
            tradingDataProcessing({
              type: 'order',
              category: 'strategy',
              key: dest,
              dataResolved: orderResolved,
            });
          }
        },
        DEFAULT_SPLIT_LENGTH,
      );
      handler.onFinish(() => {
        resolve();
      });
    });
  }

  function processTradeList(tradeList: KungfuApi.Trade[]) {
    return new Promise<void>((resolve) => {
      const handler = doSomethingWithDataSliced(
        tradeList,
        (slicedTradeList) => {
          for (let i = 0; i < slicedTradeList.length; i++) {
            const trade = slicedTradeList[i];
            if (!trade || !watcher) continue;
            const { source, dest } = trade;

            const tradeResolved = getTradeResolved(watcher, trade);

            tradingDataProcessing({
              type: 'trade',
              category: 'td',
              key: source,
              dataResolved: tradeResolved,
            });

            tradingDataProcessing({
              type: 'trade',
              category: 'strategy',
              key: dest,
              dataResolved: tradeResolved,
            });
          }
        },
        DEFAULT_SPLIT_LENGTH,
      );
      handler.onFinish(() => {
        resolve();
      });
    });
  }

  //匹配order更新orderStat
  function updateRemainingOrderStats(orderStatsMap: OrderStatsMap) {
    const orderStatOfOrderKeys = Object.keys(orderStatsMap.order);
    const tdKeys = Object.keys(tradingDataKeeper.order.td);
    const strategyKeys = Object.keys(tradingDataKeeper.order.strategy);

    for (let i = 0; i < orderStatOfOrderKeys.length; i++) {
      const key = orderStatOfOrderKeys[i];
      const orderStat = orderStatsMap.order[key];

      if (!orderStat || !orderStat.insert_time) continue;

      for (let j = 0; j < tdKeys.length; j++) {
        const source = tdKeys[j];
        const indexMap = tradingDataKeeper.order.td[source];
        if (!indexMap) continue;
        const order = indexMap.getValue(
          orderStat.insert_time,
          orderStat.order_id,
        );

        if (!order) {
          unMatchedOrderStatMap[key] = orderStat;
          continue;
        }

        const orderResolved = getOrderResolved(
          watcher as KungfuApi.Watcher,
          order,
          orderStat,
        );

        sortDataMap.set(`${'order'}_${'td'}_${source}`, indexMap);
        const isFinished = !UnfinishedOrderStatus.includes(
          orderResolved.status,
        );
        indexMap.updateKeyWithValue(orderResolved, 'order', isFinished);
      }

      for (let k = 0; k < strategyKeys.length; k++) {
        const dest = strategyKeys[k];
        const orderStat = orderStatsMap.order[key];
        if (!orderStat || !orderStat.insert_time) continue;
        const indexMap = tradingDataKeeper.order.strategy[dest];
        if (!indexMap) continue;

        const order = indexMap.getValue(
          orderStat.insert_time,
          orderStat.order_id,
        );
        if (!order) {
          unMatchedOrderStatMap[key] = orderStat;
          continue;
        }

        const orderResolved = getOrderResolved(
          watcher as KungfuApi.Watcher,
          order,
          orderStat,
        );

        sortDataMap.set(`${'order'}_${'strategy'}_${dest}`, indexMap);
        const isFinished = !UnfinishedOrderStatus.includes(
          orderResolved.status,
        );

        indexMap.updateKeyWithValue(orderResolved, 'order', isFinished);
      }
    }
  }

  async function dealTradingDataOptimized(
    orderList: KungfuApi.Order[],
    tradeList: KungfuApi.Trade[],
    orderStatList: KungfuApi.OrderStat[],
  ) {
    const orderStatsMap = extractOrderStats(orderStatList);
    await Promise.all([
      processOrderList(orderList, orderStatsMap),
      processTradeList(tradeList),
    ]);
    await updateRemainingOrderStats(orderStatsMap);
  }

  return {
    startWatcherSyncTask,
    releaseQueue,
  };
}

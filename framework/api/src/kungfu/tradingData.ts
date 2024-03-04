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
import { hashSingleUKey } from '@kungfu-trader/kungfu-js-api/kungfu';
import { UnfinishedOrderStatus } from '@kungfu-trader/kungfu-js-api/config/tradingConfig';
import BTree from 'sorted-btree';

export class DynamicTradingDataIndexedMap<K extends string | number, V> {
  private tradingDataType: 'order' | 'trade';
  private keyValueMap: { [key in K]?: V };
  private commonKeyIndexMap: { [key in K]?: number };
  private unfinishedKeyIndexMap: { [key in K]?: number };
  private commonList: V[];
  private unfinishedList: V[];
  private updateFinishedIndexList: number[] = [];
  // private maxCommonListLength = 50000;
  private commonTree: BTree<any>;
  private unfinishedTree: BTree<any>;
  private sortStr1 = '';
  private sortStr2 = '';

  constructor(type: 'order' | 'trade', _maxLength = 500) {
    this.tradingDataType = type;
    this.keyValueMap = {};
    this.commonKeyIndexMap = {};
    this.unfinishedKeyIndexMap = {};
    this.commonList = [];
    this.unfinishedList = [];

    if (this.tradingDataType === 'order') {
      this.sortStr1 = 'insert_time';
      this.sortStr2 = 'order_id';
    } else {
      this.sortStr1 = 'trade_time';
      this.sortStr2 = 'trade_id';
    }
    this.commonTree = new BTree(undefined, (a, b) => {
      if (a[this.sortStr1] > b[this.sortStr1]) return 1;
      else if (a[this.sortStr1] < b[this.sortStr1]) return -1;
      else {
        return a[this.sortStr2]
          .toString()
          .localeCompare(b[this.sortStr2].toString());
      }
    });
    this.unfinishedTree = new BTree(undefined, (a, b) => {
      if (a[this.sortStr1] > b[this.sortStr1]) return 1;
      else if (a[this.sortStr1] < b[this.sortStr1]) return -1;
      else {
        return a[this.sortStr2]
          .toString()
          .localeCompare(b[this.sortStr2].toString());
      }
    });
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
    this.commonTree.set(
      {
        [this.sortStr1]: value[this.sortStr1],
        [this.sortStr2]: value[this.sortStr2],
      },
      value,
      true,
    );

    this.keyValueMap[key] = value;
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
  updateKeyWithValue(key: K, value: V, type: string, isFinished = true): void {
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

    const unfinishedCorrectIndex = this.getUnfinishedListIndexForKey(key);
    if (unfinishedCorrectIndex !== undefined) {
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
  }

  getCommonListIndexForKey(key: K): number | undefined {
    const index = this.commonKeyIndexMap[key];

    if (index === undefined) {
      return undefined;
    }

    return Number(index);
  }

  getUnfinishedListIndexForKey(key: K): number | undefined {
    const index = this.unfinishedKeyIndexMap[key];
    if (index === undefined) {
      return;
    }
    return Number(index) - this.countSmallerNumbers(index);
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

  sortCommonList(_compareFn: (a: V, b: V) => number): void {
    this.commonList = this.commonTree
      .valuesArray()
      .reverse()
      .slice(0, globalThis.tradingDataLength);
  }

  sortUnfinishedList(_compareFn: (a: V, b: V) => number): void {
    this.unfinishedList = this.unfinishedTree
      .valuesArray()
      .reverse()
      .slice(0, globalThis.tradingDataLength);
  }
}

const { startWatcherSyncTask } = useWatcher();
export const tradingDataSubject = new Subject<{
  watcher: KungfuApi.Watcher;
  tradingData: KungfuApi.tradingData;
}>();

export const triggerStartStep = (stepInterval = 2000) => {
  startWatcher();
  startWatcherSyncTask(stepInterval, (watcher, tradingData) => {
    tradingDataSubject.next({ watcher, tradingData });
  });
};

type TradingDataList = {
  orderList: KungfuApi.Order[];
  orderStatList: KungfuApi.OrderStat[];
  tradeList: KungfuApi.Trade[];
};

type ProcessingData = {
  type: 'order' | 'trade';
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
  tradingData: KungfuApi.tradingData,
) => void;

const DEFAULT_SPLIT_LENGTH = 100;
// const DEFAULT_TRADING_DATA_LENGTH = 50000;
globalThis.tradingDataLength = 50000;
globalThis.posGlobalLength = 50000;

const bestEventLoopTask =
  typeof window !== 'undefined'
    ? globalThis.requestAnimationFrame
    : setImmediate;
globalThis.stopSync = false;
export function useWatcher() {
  let dataQueue: TradingDataList[] = [];
  let isProcessing = false; // 标记是否正在处理队列中的数据
  const tradingData: KungfuApi.tradingData = {
    order: {
      td: {},
      strategy: {},
    },
    trade: {
      td: {},
      strategy: {},
    },
    tradingDataForEach: async function (
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
      const indexMapList: KungfuApi.KfDynamicTradingDataIndexedMap<
        string,
        KungfuApi.OrderResolved | KungfuApi.TradeResolved
      >[] = groupFilterKeys
        ? groupFilterKeys
            .map((item) => this[tradingDataType][tradingDataGroup][item])
            .filter((item) => item)
        : Object.values(this[tradingDataType][tradingDataGroup]);
      const listMap: (KungfuApi.OrderResolved | KungfuApi.TradeResolved)[][] =
        [];

      const everyLatestDataResolved = indexMapList.map((item, index) => {
        const list = item[listGetter]();
        const firstData = list[0];
        if (!firstData) return;
        listMap[index] = list;
        return { data: firstData, index, position: 0 };
      });

      const compare = (a, b) => {
        const str = a.data.trade_id ? 'trade_time' : 'insert_time';
        return Number(b.data[str]) - Number(a.data[str]);
      };

      const processChunk = (chunk) => {
        return new Promise<void>((resolve) => {
          bestEventLoopTask(() => {
            const maxIterations = 500;
            for (let i = 0; chunk.length > 0 && i < maxIterations; i++) {
              chunk.sort(compare);
              const maxItem = chunk.shift();
              if (!maxItem) break;

              const isProcessed = callback(maxItem.data);
              if (!isProcessed) {
                flag = false;
                break;
              }

              const nextPosition = maxItem.position + 1;
              const nextData = listMap[maxItem.index][nextPosition];
              if (nextData) {
                chunk.push({
                  data: nextData,
                  index: maxItem.index,
                  position: nextPosition,
                });
              }
            }
            resolve();
          });
        });
      };

      while (everyLatestDataResolved.length > 0 && flag) {
        await processChunk(everyLatestDataResolved);
      }
    },
  };

  globalThis.tradingData = tradingData;

  const sortDataMap = new Map<
    string,
    KungfuApi.KfDynamicTradingDataIndexedMap<
      string,
      KungfuApi.OrderResolved | KungfuApi.TradeResolved
    >
  >();

  //添加或更新数据
  function tradingDataProcessing(
    data: ProcessingData,
    dataType: 'order' | 'trade',
  ) {
    const { category, key, orderUKey, orderResolved } = data;

    // let defaultLength = 0;

    // defaultLength = DEFAULT_TRADING_DATA_LENGTH;

    if (!tradingData[dataType][category][key]) {
      tradingData[dataType][category][key] = new DynamicTradingDataIndexedMap<
        string,
        KungfuApi.OrderResolved
      >(dataType);
    }

    const target = tradingData[dataType][category][key];
    sortDataMap.set(`${dataType}_${category}_${key}`, target);
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
    if (globalThis.stopSync) return;
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
      await drainStatesBySync();
      callBack && callBack(watcher as KungfuApi.Watcher, tradingData);
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
      const compare = (a, b) => {
        const str = a.trade_id ? 'trade_time' : 'insert_time';
        return Number(b[str]) - Number(a[str]);
      };
      while (dataQueue.length > 0) {
        const data = dataQueue.shift();
        if (
          data &&
          (data.orderList.length > 0 ||
            data.tradeList.length > 0 ||
            data.orderStatList.length > 0)
        ) {
          sortDataMap.clear();
          await dealtradingDataOptimized(
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

          const { source, dest, uid_key: orderUKey } = order;
          const orderIndexMap = tradingData.order.td[source];

          const OldOrderResolved = orderIndexMap
            ? orderIndexMap.getValueForKey(orderUKey) || null
            : null;

          const orderResolved = getOrderResolved(
            watcher,
            OldOrderResolved,
            order,
            orderStatsMap.order[orderUKey] || null,
          );

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
          const { source, dest } = trade;
          const orderUKey = hashSingleUKey(trade.order_id);

          const indexMap = tradingData.trade.td[source];
          const oldTradeResolved = indexMap
            ? indexMap.getValueForKey(orderUKey) || null
            : null;
          const tradeResolved = getTradeResolved(
            watcher,
            oldTradeResolved,
            trade,
            orderStatsMap.trade[orderUKey] || null,
          );

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
        }

        await Promise.all(tasks);
      },
      DEFAULT_SPLIT_LENGTH,
    );
  }

  //匹配order更新orderStat
  function updateRemainingOrderStats(orderStatsMap: OrderStatsMap) {
    const orderStatOfOrderKeys = Object.keys(orderStatsMap.order);
    const tdKeys = Object.keys(tradingData.order.td);
    const strategyKeys = Object.keys(tradingData.order.strategy);

    for (let i = 0; i < orderStatOfOrderKeys.length; i++) {
      const key = orderStatOfOrderKeys[i];
      const orderStat = orderStatsMap.order[key];
      if (!orderStat || !orderStat.insert_time) continue;

      for (let j = 0; j < tdKeys.length; j++) {
        const source = tdKeys[j];
        const indexMap = tradingData.order.td[source];
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

      for (let k = 0; k < strategyKeys.length; k++) {
        const dest = strategyKeys[k];
        const orderStat = orderStatsMap.order[key];
        if (!orderStat || !orderStat.insert_time) continue;
        const indexMap = tradingData.order.strategy[dest];
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
        const tdKeys = Object.keys(tradingData.trade.td);
        for (let j = 0; j < tdKeys.length; j++) {
          const source = tdKeys[j];
          const indexMap = tradingData.trade.td[source];
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

        const strategyKeys = Object.keys(tradingData.trade.strategy);
        for (let k = 0; k < strategyKeys.length; k++) {
          const dest = strategyKeys[k];
          const orderStat = orderStatsMap.trade[key];
          if (!orderStat || !orderStat.trade_time) continue;
          const indexMap = tradingData.trade.strategy[dest];
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

  async function dealtradingDataOptimized(
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

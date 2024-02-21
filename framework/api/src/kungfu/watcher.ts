import { getRendererProcessId, kf } from './index';
import { KF_RUNTIME_DIR } from '@kungfu-trader/kungfu-js-api/config/pathConfig';
import { DynamicIndexedMap } from '@kungfu-trader/kungfu-js-api/utils/classUtils';
import { getKfGlobalSettingsValue } from '@kungfu-trader/kungfu-js-api/config/globalSettings';
import {
  setTimerPromiseTask,
  // statTime,
  // statTimeEnd,
} from '@kungfu-trader/kungfu-js-api/utils/commonUtils';
import { kfLogger } from '@kungfu-trader/kungfu-js-api/utils/logUtils';
import { booleanProcessEnv } from '@kungfu-trader/kungfu-js-api/utils/commonUtils';
import { getOrderResolved } from '@kungfu-trader/kungfu-js-api/utils/tradingUtils';
import { UnfinishedOrderStatus } from '@kungfu-trader/kungfu-js-api/config/tradingConfig';

export const watcher = ((): KungfuApi.Watcher | null => {
  if (process.env.APP_TYPE !== 'renderer') {
    if (process.env.APP_TYPE !== 'service') {
      if (process.env.APP_TYPE !== 'cli') {
        return null;
      }
    }
  }

  if (process.env.APP_TYPE === 'renderer') {
    if (process.env.APP_ID !== 'app') {
      return null;
    }
  }

  kfLogger.info(
    'Init Watcher',
    'APP_TYPE',
    process.env.APP_TYPE || 'undefined',
    'UI_EXT_TYPE',
    process.env.UI_EXT_TYPE || 'undefined',
    'APP_ID',
    process.env.APP_ID || 'undefined',
  );

  const bypassRestore =
    booleanProcessEnv(process.env.RELOAD_AFTER_CRASHED) ||
    booleanProcessEnv(process.env.BY_PASS_RESTORE);
  const globalSetting = getKfGlobalSettingsValue();
  const bypassAccounting =
    process.env.BY_PASS_ACCOUNTING ??
    globalSetting?.performance?.bypassAccounting ??
    false;
  const bypassTradingData =
    process.env.BY_PASS_TRADINGDATA ??
    globalSetting?.performance?.bypassTradingData ??
    false;
  const refreshTradingDataBeforeSync =
    process.env.REFRESH_LEDGER_BEFORE_SYNC ?? true;

  const bypassRefreshBook =
    process.env.BY_PASS_REFRESHBOOK ??
    globalSetting?.performance?.bypassRefreshBook ??
    false;

  const millisecondsSleepAfterStep =
    process.env.MILLISECONDS_SLEEP_AFTER_STEP ?? 50;

  kfLogger.info('bypassRestore', bypassRestore);
  kfLogger.info('bypassAccounting', bypassAccounting);
  kfLogger.info('bypassTradingData', bypassTradingData);
  kfLogger.info('refreshTradingDataBeforeSync', refreshTradingDataBeforeSync);
  kfLogger.info('bypassRefreshBook', bypassRefreshBook);
  kfLogger.info('millisecondsSleepAfterStep', millisecondsSleepAfterStep);

  return kf.watcher(
    KF_RUNTIME_DIR,
    getRendererProcessId(),
    !!bypassRestore,
    !!bypassAccounting,
    !!bypassTradingData,
    !!refreshTradingDataBeforeSync,
    !!bypassRefreshBook,
    +millisecondsSleepAfterStep,
  );
})();

export const startWatcher = () => {
  if (watcher === null) return;
  watcher.start();
};

type TradingDataList = {
  orderList: KungfuApi.Order[];
  tradeList: KungfuApi.Trade[];
  positionList: KungfuApi.Position[];
};

export function useWatcher() {
  let dataQueue: TradingDataList[] = [];
  let isProcessing = false; // 标记是否正在处理队列中的数据
  const tradingDataObject: KungfuApi.TradingDataObject = {
    order: {
      td: {},
      strategy: {},
    },
    trade: {
      tradeMap: new Map(),
      tradeList: [],
      addedTradeList: [],
      updatedTradeList: [],
    },
    position: {
      positionMap: new Map(),
      positionList: [],
      addedPositionList: [],
      updatedPositionList: [],
    },
  };
  const startWatcherSyncTask = (
    interval = 1000,
    callback?: (
      watcher: KungfuApi.Watcher,
      tradingDataObject: KungfuApi.TradingDataObject,
      update: boolean,
    ) => void,
  ) => {
    if (watcher === null) return;

    // const DEFAULT_SPLIT_LENGTH = 1000;

    return setTimerPromiseTask(async () => {
      watcher.sync();
      const orderStatList = Object.values(watcher.ledger.OrderStat);
      const orderList = Object.values(watcher.ledger.Order);
      const tradeList = Object.values(watcher.ledger.Trade);
      const positionList = Object.values(watcher.ledger.Position);
      console.log('orderStatList', orderList.length, orderStatList.length);

      dataQueue.push({ orderList, tradeList, positionList });

      // 开始处理队列中的数据
      if (!isProcessing) {
        isProcessing = true;
        callback && processQueue(callback);
      }
      return true;
    }, interval);
  };
  async function processQueue(callback) {
    try {
      while (dataQueue.length > 0) {
        const data = dataQueue.shift();
        if (data && data.orderList.length > 0) {
          console.time('dealTradingDataObject');
          await dealTradingDataObjectOptimized(data.orderList);
          console.timeEnd('dealTradingDataObject');
          if (watcher && tradingDataObject) {
            console.log('obj', tradingDataObject);
            callback && callback(watcher, tradingDataObject, true);
          }
        } else {
          if (watcher && tradingDataObject) {
            callback && callback(watcher, tradingDataObject, false);
          }
        }
      }
    } catch (error) {
      console.error('An error occurred while processing the queue:', error);
    } finally {
      isProcessing = false;
    }
  }

  async function dealTradingDataObjectOptimized(orderList: KungfuApi.Order[]) {
    const markObject = {}; // 用于标记是否已处理

    //处理 orderList
    await doSomethingWithDataSliced(
      orderList,
      async (slicedOrderList, _sliceIndex) => {
        for (const order of slicedOrderList) {
          if (!order) continue;
          if (!watcher) return;
          const { source, dest, uid_key: orderUKey } = order;

          const orderResolved = getOrderResolved(
            watcher as KungfuApi.Watcher,
            order,
            watcher.ledger.OrderStat[orderUKey] || null,
          );
          await processOrder(
            'td',
            source,
            orderUKey,
            orderResolved,
            markObject,
          );
          await processOrder(
            'strategy',
            dest,
            orderUKey,
            orderResolved,
            markObject,
          );
        }
      },

      1000,
    );
  }

  function processOrder(
    type: string,
    key: number,
    orderUKey: string,
    orderResolved: KungfuApi.OrderResolved,
    markObject: Record<string, boolean>,
  ) {
    if (!tradingDataObject.order[type][key]) {
      tradingDataObject.order[type][key] = {
        orderIndexMap: new DynamicIndexedMap<string, KungfuApi.OrderResolved>(),
        addedOrderList: [],
        addFinishedOrderList: [[], 0],
        deletedOrderList: [],
        updatedOrderList: [[], []],
      };
    }

    const target = tradingDataObject.order[type][key];
    if (!markObject[key]) {
      target.addedOrderList = [];
      target.updatedOrderList = [[], []];
      target.addFinishedOrderList = [[], 0];
      target.deletedOrderList = [];

      markObject[key] = true; // 标记已处理
    }

    if (target.orderIndexMap.hasKey(orderUKey)) {
      const index = target.orderIndexMap.getOrderStatus(orderUKey);
      if (index <= 0 && !UnfinishedOrderStatus.includes(orderResolved.status)) {
        target.orderIndexMap.deleteUnfinishedKeyAndValue(orderUKey);

        target.orderIndexMap.insertKeyWithValue(
          orderUKey,
          orderResolved,
          false,
        );
      } else {
        target.orderIndexMap.updateKeyWithValue(orderUKey, orderResolved);
      }
    } else {
      if (UnfinishedOrderStatus.includes(orderResolved.status)) {
        target.orderIndexMap.insertKeyWithValue(orderUKey, orderResolved);
      } else {
        target.orderIndexMap.insertKeyWithValue(
          orderUKey,
          orderResolved,
          false,
        );
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
  doSomethingCallback: (dataItem: T, index: number, sliceIndex: number) => void,
  sliceLength = 1000,
) => {
  return new Promise<void>((resolve, reject) => {
    let i = 0,
      sliceIndex = 0;
    const len = data.length;
    const bestEventLoopTask = window
      ? window.requestAnimationFrame
      : setTimeout;
    const runner = () => {
      bestEventLoopTask(async () => {
        for (let j = 0; j < sliceLength && i < len; i++, j++) {
          await Promise.resolve(
            doSomethingCallback(data[i], i, sliceIndex),
          ).catch(reject);
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
  doSomethingCallback: (dataSliced: T[], sliceIndex: number) => void,
  sliceLength = 1000,
) => {
  const dataSliced: T[] = [];
  return new Promise<void>((resolve, reject) => {
    dataOperationBySliceInEventLoop(
      data,
      async (dataItem, index, sliceIndex) => {
        dataSliced.push(dataItem);

        if (dataSliced.length === sliceLength || index === data.length - 1) {
          await Promise.resolve(
            doSomethingCallback(dataSliced, sliceIndex),
          ).catch(reject);
          dataSliced.length = 0;
        }

        if (index === data.length - 1) resolve();
      },
      sliceLength,
    );
  });
};

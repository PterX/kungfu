import { UnfinishedOrderStatus } from './../config/tradingConfig';
import dayjs, { Dayjs } from 'dayjs';
import dayjsBusinessDays from 'dayjs-business-days';
import { kungfu } from '@kungfu-trader/kungfu-core';
import { KF_RUNTIME_DIR } from '../config/pathConfig';
import {
  dealAssetPrice,
  dealCurrency,
  dealDirection,
  dealHedgeFlag,
  dealInstrumentType,
  dealIsSwap,
  dealKfPrice,
  dealLocationUID,
  dealOffset,
  dealOrderStat,
  dealOrderStatus,
  dealPriceType,
  dealSide,
  dealTimeCondition,
  dealVolumeCondition,
  getIdByKfLocation,
  getMdTdKfLocationByProcessId,
  isShotable,
  isT0,
  kfLogger,
  resolveAccountId,
  resolveClientId,
  setTimerPromiseTask,
  dealOrderTriggerStatus,
  dealTOrderTriggerFlag,
} from '../utils/busiUtils';
import {
  HistoryDateEnum,
  InstrumentTypeEnum,
  CurrencyEnum,
  OrderActionFlagEnum,
  OrderTriggerTypeEnum,
} from '../typings/enums';
import { ExchangeIds, AllFinishedOrderStatus } from '../config/tradingConfig';
dayjs.extend(dayjsBusinessDays);

type DayjsWithBusinessDays = Dayjs & { isBusinessDay: () => boolean };

export const kf = kungfu();

kfLogger.info('Load kungfu node');

export const tracer = (
  kflocation: KungfuApi.KfLocation,
  read: boolean,
  write: boolean,
  startTime: bigint,
  endTime: bigint,
) => kf.tracer(kflocation, KF_RUNTIME_DIR, read, write, startTime, endTime);
export const configStore = kf.ConfigStore(KF_RUNTIME_DIR);
export const riskSettingStore = kf.RiskSettingStore(KF_RUNTIME_DIR);
export const history = kf.History(KF_RUNTIME_DIR);
export const commissionStore = kf.CommissionStore(KF_RUNTIME_DIR);
export const basketStore = kf.BasketStore(KF_RUNTIME_DIR);
export const basketInstrumentStore = kf.BasketInstrumentStore(KF_RUNTIME_DIR);
export const sessionStore = kf.SessionStore(
  getCurrentNodeLocation(),
  KF_RUNTIME_DIR,
);
export const longfist = kf.Longfist();
export const io = kf.IODevice(getCurrentNodeLocation(), KF_RUNTIME_DIR);

export function getCurrentNodeLocation(): KungfuApi.KfLocation {
  return {
    mode: 'live',
    category: 'system',
    group: 'node',
    name: getRendererProcessId(),
  };
}

export function getRendererProcessId(): string {
  const watcherId = [
    process.env.APP_TYPE,
    process.env.UI_EXT_TYPE,
    (process.env.APP_ID || '').length > 16
      ? kf.formatStringToHashHex(process.env.APP_ID || '')
      : process.env.APP_ID,
  ]
    .filter((str) => !!str)
    .join('-');
  kfLogger.info(`Renderer ProcessId ${watcherId}`);
  return watcherId;
}

export const dealKfTime = (nano: bigint, date = false): string => {
  if (nano === BigInt(0)) {
    return '--';
  }

  if (date) {
    return kf.formatTime(nano, '%m/%d %H:%M:%S.%N').slice(0, 18);
  }
  return kf.formatTime(nano, '%H:%M:%S.%N').slice(0, 12);
};

export const dealTradingDataItem = (
  item: KungfuApi.TradingDataTypes,
  watcher: KungfuApi.Watcher | null,
  isShowOrigin = false,
): Record<string, string | number | bigint> => {
  const itemResolved = { ...item } as Record<string, string | number | bigint>;
  const instrument_type =
    'instrument_type' in item
      ? item.instrument_type
      : InstrumentTypeEnum.unknown;
  const isInstrumnetShotable = isShotable(instrument_type);

  if ('order_id' in item) {
    itemResolved.order_id = item.order_id.toString();
  }

  if ('trade_id' in item) {
    itemResolved.trade_id = item.trade_id.toString();
  }

  if ('instrument_id' in item) {
    itemResolved.instrument_id = item.instrument_id.toString();
  }

  if ('trade_time' in item && !isShowOrigin) {
    itemResolved.trade_time = dealKfTime(item.trade_time, true);
  }
  if ('insert_time' in item && !isShowOrigin) {
    itemResolved.insert_time = dealKfTime(item.insert_time, true);
  }
  if ('update_time' in item && !isShowOrigin) {
    itemResolved.update_time = dealKfTime(item.update_time, true);
  }
  if ('direction' in item) {
    itemResolved.direction = dealDirection(item.direction).name;
  }
  if ('side' in item) {
    itemResolved.side = dealSide(item.side).name;
  }
  if ('offset' in item) {
    if (isInstrumnetShotable) {
      itemResolved.offset = dealOffset(item.offset).name;
    } else {
      delete itemResolved.offset;
    }
  }
  if ('status' in item) {
    itemResolved.status = dealOrderStatus(
      item.status,
      item.error_msg || '',
    ).name;
  }
  if ('price_type' in item) {
    itemResolved.price_type = dealPriceType(item.price_type).name;
  }

  if ('volume_condition' in item) {
    if (isInstrumnetShotable) {
      itemResolved.volume_condition = dealVolumeCondition(
        item.volume_condition,
      ).name;
    } else {
      delete itemResolved.volume_condition;
    }
  }

  if ('time_condition' in item) {
    if (isInstrumnetShotable) {
      itemResolved.time_condition = dealTimeCondition(item.time_condition).name;
    } else {
      delete itemResolved.time_condition;
    }
  }

  if ('instrument_type' in item) {
    itemResolved.instrument_type = dealInstrumentType(
      item.instrument_type,
    ).name;
  }
  if ('hedge_flag' in item) {
    if (isInstrumnetShotable) {
      itemResolved.hedge_flag = dealHedgeFlag(item.hedge_flag).name;
    } else {
      delete itemResolved.hedge_flag;
    }
  }
  if ('is_swap' in item) {
    if (isInstrumnetShotable) {
      itemResolved.is_swap = dealIsSwap(item.is_swap).name;
    } else {
      delete itemResolved.is_swap;
    }
  }
  if ('source' in item && 'dest' in item && watcher) {
    itemResolved.source = resolveAccountId(
      watcher,
      item.source,
      item.dest,
    ).name;
  }
  if ('dest' in item && watcher) {
    itemResolved.dest = resolveClientId(watcher, item.dest).name;
  }
  if ('holder_uid' in item && watcher) {
    itemResolved.holder_uid = dealLocationUID(watcher, item.holder_uid);
  }

  if ('currency' in item) {
    itemResolved.currency = dealCurrency(item.currency).name;
  }
  return itemResolved;
};

export const getKungfuDataByDateRange = (
  date: number | string,
  dateType = HistoryDateEnum.naturalDate, //0 natural date, 1 tradingDate
): Promise<KungfuApi.TradingData | Record<string, unknown>> => {
  const targetDate = dayjs(date).format('YYYY-MM-DD');
  const yesterdayDate = dayjs(date).add(-1, 'day').format('YYYY-MM-DD');
  const isYesterdayBusinessDay = (
    dayjs(date).add(-1, 'day') as DayjsWithBusinessDays
  ).isBusinessDay();
  const fridayDate = dayjs(date).add(-3, 'day').format('YYYY-MM-DD');
  let from =
    dateType == HistoryDateEnum.naturalDate
      ? dayjs(targetDate).format('YYYY-MM-DD HH:mm:ss')
      : isYesterdayBusinessDay
      ? dayjs(yesterdayDate).format('YYYY-MM-DD HH:mm:ss')
      : dayjs(fridayDate).format('YYYY-MM-DD HH:mm:ss');
  let to = dayjs(targetDate).add(1, 'day').format('YYYY-MM-DD HH:mm:ss');

  if (dateType == HistoryDateEnum.tradingDate) {
    from = dayjs(from).add(16, 'hour').format('YYYY-MM-DD HH:mm:ss');
    to = dayjs(to).add(-8, 'hour').format('YYYY-MM-DD HH:mm:ss');
  }

  kfLogger.info(
    'is yesterday bussiness day',
    yesterdayDate,
    isYesterdayBusinessDay,
  );
  kfLogger.info('Export data', from, to);

  return new Promise((resolve, reject) => {
    const timer = setTimeout(() => {
      //by trading date
      const kungfuDataToday = history.selectPeriod(from, to);

      if (!kungfuDataToday) return reject(new Error('database_locked'));

      resolve(kungfuDataToday);
      clearTimeout(timer);
    }, 160);
  });
};

export const getKungfuHistoryData = (
  date: string,
  dateType: HistoryDateEnum,
  tradingDataTypeName: KungfuApi.TradingDataTypeName | 'all',
  kfLocation?: KungfuApi.KfLocation,
): Promise<{
  tradingData: KungfuApi.TradingData;
}> => {
  return getKungfuDataByDateRange(date, dateType).then(
    (tradingData: KungfuApi.TradingData | Record<string, unknown>) => {
      if (tradingDataTypeName === 'all') {
        return {
          tradingData: tradingData as KungfuApi.TradingData,
        };
      }

      if (!kfLocation) {
        return {
          tradingData: tradingData as KungfuApi.TradingData,
        };
      }

      return {
        tradingData: tradingData as KungfuApi.TradingData,
      };
    },
  );
};

export const kfRequestMarketData = (
  watcher: KungfuApi.Watcher | null,
  exchangeId: string,
  instrumentId: string,
  mdLocation: KungfuApi.KfLocation,
): Promise<boolean> => {
  if (!watcher) {
    return Promise.reject(new Error('Watcher is NULL'));
  }

  if (!watcher.isLive()) {
    return Promise.reject(new Error(`Watcher is not live`));
  }

  if (!watcher.isReadyToInteract(mdLocation)) {
    const sourceId = getIdByKfLocation(mdLocation);
    return Promise.reject(new Error(`Md ${sourceId} not ready`));
  }

  return Promise.resolve(
    watcher.requestMarketData(mdLocation, exchangeId, instrumentId),
  );
};

export const kfCancelOrder = (
  watcher: KungfuApi.Watcher | null,
  order: KungfuApi.Order,
  orderActionFlag: OrderActionFlagEnum,
): Promise<bigint> => {
  if (!watcher) {
    return Promise.reject(new Error(`Watcher is NULL`));
  }

  if (!watcher.isLive()) {
    return Promise.reject(new Error(`Watcher is not live`));
  }

  const { order_id, dest, source } = order;
  const sourceLocation = watcher.getLocation(source);
  const destLocation = watcher.getLocation(dest);

  if (!watcher.isReadyToInteract(sourceLocation)) {
    const accountId = getIdByKfLocation(sourceLocation);
    return Promise.reject(new Error(`Td ${accountId} not ready`));
  }

  const orderAction: KungfuApi.OrderAction = {
    ...longfist.types.OrderAction(),
    action_flag: orderActionFlag,
    order_id,
  };

  if (!destLocation) {
    return Promise.resolve(watcher.cancelOrder(orderAction, sourceLocation));
  }

  return Promise.resolve(
    watcher.cancelOrder(orderAction, sourceLocation, destLocation),
  );
};

export const kfCancelOrderTrigger = (
  watcher: KungfuApi.Watcher | null,
  order: KungfuApi.OrderTriggerResolved,
  tdLocation: KungfuApi.KfLocation,
): Promise<bigint> => {
  if (!watcher) {
    return Promise.reject(new Error(`Watcher is NULL`));
  }

  if (!watcher.isLive()) {
    return Promise.reject(new Error(`Watcher is not live`));
  }

  const { source, trigger_id } = order;
  const sourceLocation = watcher.getLocation(source);

  if (!watcher.isReadyToInteract(tdLocation)) {
    const accountId = getIdByKfLocation(tdLocation);
    return Promise.reject(new Error(`Td ${accountId} not ready`));
  }

  const orderAction: KungfuApi.OrderTriggerAction = {
    ...longfist.types.OrderTriggerAction(),
    trigger_id,
  };

  return Promise.resolve(
    watcher.cancelOrderTrigger(orderAction, sourceLocation),
  );
};

export const kfCancelOrderUtilFinished = (
  watcher: KungfuApi.Watcher,
  order: KungfuApi.Order,
) => {
  return new Promise<KungfuApi.Order>((resolve, reject) => {
    if (!UnfinishedOrderStatus.includes(order.status)) return resolve(order);

    kfCancelOrder(watcher, order, OrderActionFlagEnum.Cancel)
      .then(() => {
        const { clearLoop } = setTimerPromiseTask(() => {
          const targetOrder = (watcher as KungfuApi.Watcher).ledger.Order[
            order.uid_key
          ];
          if (
            targetOrder &&
            AllFinishedOrderStatus.includes(targetOrder.status)
          ) {
            clearLoop();
            resolve(targetOrder);
          }
          return Promise.resolve();
        }, 160);
      })
      .catch((err) => reject(err));
  });
};

export const kfCancelAllOrders = (
  watcher: KungfuApi.Watcher | null,
  orders: KungfuApi.Order[],
): Promise<bigint[]> => {
  if (!watcher) {
    return Promise.reject(new Error(`Watcher is NULL`));
  }

  if (!watcher.isLive()) {
    return Promise.reject(new Error(`Watcher is not live`));
  }

  const cancelOrderTasks = orders.map(
    (item: KungfuApi.Order): Promise<bigint> => {
      return kfCancelOrder(watcher, item, OrderActionFlagEnum.Cancel);
    },
  );

  return Promise.all(cancelOrderTasks);
};

export const kfCancelAllOrdersTrigger = (
  watcher: KungfuApi.Watcher | null,
  orders: KungfuApi.OrderTriggerResolved[],
  tdLocation: KungfuApi.KfLocation,
): Promise<bigint[]> => {
  if (!watcher) {
    return Promise.reject(new Error(`Watcher is NULL`));
  }

  if (!watcher.isLive()) {
    return Promise.reject(new Error(`Watcher is not live`));
  }

  if (!watcher.isReadyToInteract(tdLocation)) {
    const accountId = getIdByKfLocation(tdLocation);
    return Promise.reject(new Error(`Td ${accountId} not ready`));
  }

  const cancelOrderTasks = orders.map(
    (item: KungfuApi.OrderTriggerResolved): Promise<bigint> => {
      return kfCancelOrderTrigger(watcher, item, tdLocation);
    },
  );

  return Promise.all(cancelOrderTasks);
};

export const kfMakeOrder = (
  watcher: KungfuApi.Watcher | null,
  makeOrderInput: KungfuApi.MakeOrderInput,
  tdLocation: KungfuApi.KfLocation,
  strategyLocation?: KungfuApi.KfLocation,
): Promise<bigint> => {
  if (!watcher) {
    return Promise.reject(new Error('Watcher is NULL'));
  }

  if (!watcher.isLive()) {
    return Promise.reject(new Error(`Watcher is not live`));
  }

  if (!watcher.isReadyToInteract(tdLocation)) {
    const accountId = getIdByKfLocation(tdLocation);
    return Promise.reject(new Error(`Td ${accountId} not ready`));
  }

  const now = watcher.now();
  const orderInput: KungfuApi.OrderInput = {
    ...longfist.types.OrderInput(),
    ...makeOrderInput,
    block_id: BigInt(0),
    limit_price: makeOrderInput.limit_price || 0,
    frozen_price: makeOrderInput.limit_price || 0,
    volume: BigInt(makeOrderInput.volume),
    insert_time: now,
  };

  if (strategyLocation) {
    //设置orderInput的parentid, 来标记该order为策略手动下单
    return Promise.resolve(
      watcher.issueOrder(orderInput, tdLocation, strategyLocation),
    );
  } else {
    return Promise.resolve(watcher.issueOrder(orderInput, tdLocation));
  }
};

export const kfOrderTrigger = (
  watcher: KungfuApi.Watcher | null,
  makeOrderTriggerInput: KungfuApi.MakeOrderTriggerInput,
  tdLocation: KungfuApi.KfLocation,
): Promise<bigint> => {
  if (!watcher) {
    return Promise.reject(new Error('Watcher is NULL'));
  }

  if (!watcher.isLive()) {
    return Promise.reject(new Error(`Watcher is not live`));
  }

  if (!watcher.isReadyToInteract(tdLocation)) {
    const accountId = getIdByKfLocation(tdLocation);
    return Promise.reject(new Error(`Td ${accountId} not ready`));
  }

  const now = watcher.now();
  const orderTriggerInput: KungfuApi.OrderTriggerInput = {
    ...longfist.types.OrderTriggerInput(),
    ...makeOrderTriggerInput,
    limit_price: makeOrderTriggerInput.limit_price || 0,
    volume: BigInt(makeOrderTriggerInput.volume),
    insert_time: now,
    trigger_type: OrderTriggerTypeEnum.ParkedOrder,
  };

  return Promise.resolve(
    watcher.issueOrderTrigger(orderTriggerInput, tdLocation),
  );
};

export const kfRefreshOrderTrigger = (
  watcher: KungfuApi.Watcher | null,
  msgType: number,
  tdLocation: KungfuApi.KfLocation,
): Promise<boolean> => {
  if (!watcher) {
    return Promise.reject(new Error('Watcher is NULL'));
  }

  if (!watcher.isLive()) {
    return Promise.reject(new Error(`Watcher is not live`));
  }

  return Promise.resolve(watcher.issueMark(msgType, tdLocation));
};

export const kfMakeBlockOrder = async (
  watcher: KungfuApi.Watcher | null,
  blockMessage: KungfuApi.BlockMessage,
  makeOrderInput: KungfuApi.MakeOrderInput,
  tdLocation: KungfuApi.KfLocation,
  strategyLocation?: KungfuApi.KfLocation,
): Promise<bigint> => {
  if (!watcher) {
    return Promise.reject(new Error('Watcher is NULL'));
  }

  if (!watcher.isLive()) {
    return Promise.reject(new Error(`Watcher is not live`));
  }

  if (!watcher.isReadyToInteract(tdLocation)) {
    const accountId = getIdByKfLocation(tdLocation);
    return Promise.reject(new Error(`Td ${accountId} not ready`));
  }

  let block_id;
  if (blockMessage) {
    blockMessage = {
      ...blockMessage,
      is_specific: !!blockMessage.is_specific,
      match_number: BigInt(blockMessage.match_number),
      insert_time: watcher.now(),
    };
    block_id = await watcher.issueBlockMessage(blockMessage, tdLocation);
  }
  if (!block_id) {
    return Promise.reject(new Error('Get block_id failed'));
  }

  const now = watcher.now();
  const orderInput: KungfuApi.OrderInput = {
    ...longfist.types.OrderInput(),
    ...makeOrderInput,
    block_id,
    limit_price: makeOrderInput.limit_price || 0,
    frozen_price: makeOrderInput.limit_price || 0,
    volume: BigInt(makeOrderInput.volume),
    insert_time: now,
  };

  if (strategyLocation) {
    //设置orderInput的parentid, 来标记该order为策略手动下单
    return Promise.resolve(
      watcher.issueOrder(orderInput, tdLocation, strategyLocation),
    );
  } else {
    return Promise.resolve(watcher.issueOrder(orderInput, tdLocation));
  }
};

export const makeOrderByOrderTriggerInput = (
  watcher: KungfuApi.Watcher | null,
  orderInput: KungfuApi.MakeOrderTriggerInput,
  kfLocation: KungfuApi.KfLocation,
  accountId: string,
): Promise<bigint> => {
  return new Promise((resolve, reject) => {
    if (!watcher) {
      reject(new Error(`Watcher is NULL`));
      return;
    }

    if (kfLocation.category === 'td') {
      return kfOrderTrigger(watcher, orderInput, kfLocation)
        .then((order_id) => {
          resolve(order_id);
        })
        .catch((err) => {
          reject(err);
        });
    } else {
      const tdLocation = getMdTdKfLocationByProcessId(`td_${accountId || ''}`);
      if (!tdLocation) {
        reject(new Error('下单账户信息错误'));
        return;
      }
      return kfOrderTrigger(watcher, orderInput, tdLocation)
        .then((order_id) => {
          resolve(order_id);
        })
        .catch((err) => {
          reject(err);
        });
    }
  });
};

export const makeOrderByOrderInput = (
  watcher: KungfuApi.Watcher | null,
  orderInput: KungfuApi.MakeOrderInput,
  kfLocation: KungfuApi.KfLocation,
  accountId: string,
): Promise<bigint> => {
  return new Promise((resolve, reject) => {
    if (!watcher) {
      reject(new Error(`Watcher is NULL`));
      return;
    }

    if (kfLocation.category === 'td') {
      return kfMakeOrder(watcher, orderInput, kfLocation)
        .then((order_id) => {
          resolve(order_id);
        })
        .catch((err) => {
          reject(err);
        });
    } else if (kfLocation.category === 'strategy') {
      const tdLocation = getMdTdKfLocationByProcessId(`td_${accountId || ''}`);
      if (!tdLocation) {
        reject(new Error('下单账户信息错误'));
        return;
      }
      return kfMakeOrder(watcher, orderInput, tdLocation, kfLocation)
        .then((order_id) => {
          resolve(order_id);
        })
        .catch((err) => {
          reject(err);
        });
    } else {
      const tdLocation = getMdTdKfLocationByProcessId(`td_${accountId || ''}`);
      if (!tdLocation) {
        reject(new Error('下单账户信息错误'));
        return;
      }
      return kfMakeOrder(watcher, orderInput, tdLocation)
        .then((order_id) => {
          resolve(order_id);
        })
        .catch((err) => {
          reject(err);
        });
    }
  });
};

export const makeOrderByBlockMessage = (
  watcher: KungfuApi.Watcher | null,
  blockMessage: KungfuApi.BlockMessage,
  orderInput: KungfuApi.MakeOrderInput,
  kfLocation: KungfuApi.KfLocation,
  accountId: string,
): Promise<bigint> => {
  return new Promise((resolve, reject) => {
    if (!watcher) {
      reject(new Error(`Watcher is NULL`));
      return;
    }

    if (kfLocation.category === 'td') {
      return kfMakeBlockOrder(watcher, blockMessage, orderInput, kfLocation)
        .then((order_id) => {
          resolve(order_id);
        })
        .catch((err) => {
          reject(err);
        });
    } else if (kfLocation.category === 'strategy') {
      const tdLocation = getMdTdKfLocationByProcessId(`td_${accountId || ''}`);
      if (!tdLocation) {
        reject(new Error('下单账户信息错误'));
        return;
      }
      return kfMakeBlockOrder(
        watcher,
        blockMessage,
        orderInput,
        tdLocation,
        kfLocation,
      )
        .then((order_id) => {
          resolve(order_id);
        })
        .catch((err) => {
          reject(err);
        });
    } else {
      const tdLocation = getMdTdKfLocationByProcessId(`td_${accountId || ''}`);
      if (!tdLocation) {
        reject(new Error('下单账户信息错误'));
        return;
      }
      return kfMakeBlockOrder(watcher, blockMessage, orderInput, tdLocation)
        .then((order_id) => {
          resolve(order_id);
        })
        .catch((err) => {
          reject(err);
        });
    }
  });
};

export const makeOrderByBasketInstruments = (
  watcher: KungfuApi.Watcher | null,
  parentId: bigint,
  basketOrderInput: KungfuApi.BasketOrderInput,
  basketInstruments: KungfuApi.BasketInstrumentForOrder[],
  tdLocation: KungfuApi.KfLocation,
) => {
  if (!watcher) {
    return Promise.reject(new Error(`Watcher is NULL`));
  }

  if (!watcher.isLive()) {
    return Promise.reject(new Error(`Watcher is not live`));
  }

  if (!watcher.isReadyToInteract(tdLocation)) {
    const accountId = getIdByKfLocation(tdLocation);
    return Promise.reject(new Error(`Td ${accountId} not ready`));
  }

  const makeOrderTasks = basketInstruments.map((basketInstrument) => {
    const makeOrderInput: KungfuApi.MakeOrderInput = {
      ...longfist.types.OrderInput(),
      parent_id: parentId,
      instrument_id: `${basketInstrument.instrument_id}`,
      exchange_id: `${basketInstrument.exchange_id}`,
      instrument_type: +basketInstrument.instrument_type,
      side: +basketInstrument.sideResolved,
      offset: +basketInstrument.offsetResolved,
      price_type: +basketOrderInput.price_type,
      limit_price: +basketInstrument.priceResolved || 0,
      volume: basketInstrument.volumeResolved,
    };

    return kfMakeOrder(
      watcher,
      makeOrderInput,
      tdLocation,
      basketInstrument.strategyLocation,
    );
  });

  return Promise.all(makeOrderTasks);
};

export const makeOrderByBasketTrade = (
  watcher: KungfuApi.Watcher | null,
  basket: KungfuApi.Basket,
  basketOrderInput: KungfuApi.BasketOrderInput,
  basketInstruments: KungfuApi.BasketInstrumentForOrder[],
  kfLocation: KungfuApi.KfLocation,
) => {
  if (!watcher) {
    return Promise.reject(new Error(`Watcher is NULL`));
  }

  if (!watcher.isLive()) {
    return Promise.reject(new Error(`Watcher is not live`));
  }

  if (!watcher.isReadyToInteract(kfLocation)) {
    const accountId = getIdByKfLocation(kfLocation);
    return Promise.reject(new Error(`Td ${accountId} not ready`));
  }

  if (!basketInstruments.length) return Promise.resolve();

  const now = watcher.now();
  const basketOrder: KungfuApi.BasketOrder = {
    ...longfist.types.BasketOrder(),
    parent_id: BigInt(basket.id),
    insert_time: now,
    side: +basketOrderInput.side,
    price_type: +basketOrderInput.price_type,
    price_level: +basketOrderInput.price_level,
    price_offset: +basketOrderInput.price_offset,
  };

  const parent_id = watcher.issueBasketOrder(basketOrder, kfLocation);

  return makeOrderByBasketInstruments(
    watcher,
    parent_id,
    basketOrderInput,
    basketInstruments,
    kfLocation,
  );
};

const ukeyCacheMap = new Map<string, string>();
export const hashUkey = (...args: Array<string | number>) => {
  const cacheKey = args.map((arg) => `${arg}`).join('_');
  if (!ukeyCacheMap.has(cacheKey))
    ukeyCacheMap.set(
      cacheKey,
      args
        .reduce<bigint>((pre, cur) => pre ^ BigInt(kf.hash(cur)), 0n)
        .toString(16)
        .padStart(16, '0'),
    );

  return ukeyCacheMap.get(cacheKey) || '';
};

export const hashInstrumentUKey = (
  instrumentId: string,
  exchangeId: string,
): string => {
  return hashUkey(instrumentId, exchangeId);
};

export const hashInstrumentFactorUKey = (
  instrumentId: string,
  exchangeId: string,
  accountUID: number,
): string => {
  return hashUkey(instrumentId, exchangeId, accountUID);
};

export const getOrderLatencyDataByOrderStat = (
  order: KungfuApi.Order,
  orderStats: KungfuApi.DataTable<KungfuApi.OrderStat>,
  price_precision?: number,
) => {
  const latencyData = dealOrderStat(orderStats, order.uid_key) || {
    latencySystem: '--',
    latencyNetwork: '--',
    avg_price: 0,
  };
  return {
    latency_system: latencyData.latencySystem,
    latency_network: latencyData.latencyNetwork,
    avg_price: latencyData.avg_price,
    avg_price_resolved: dealKfPrice(latencyData.avg_price, price_precision),
  };
};

export const dealOrder = (
  watcher: KungfuApi.Watcher,
  order: KungfuApi.Order,
  isHistory = false,
  pricePrecision = 4,
): KungfuApi.OrderResolvedWithoutStat => {
  const sourceResolvedData = resolveAccountId(
    watcher,
    order.source,
    order.dest,
  );
  const destResolvedData = resolveClientId(watcher, order.dest);
  const statusData = dealOrderStatus(order.status, order.error_msg);
  return {
    ...order,
    source: order.source,
    dest: order.dest,
    uid_key: order.uid_key,
    source_resolved_data: sourceResolvedData,
    dest_resolved_data: destResolvedData,
    source_uname: sourceResolvedData.name,
    dest_uname: destResolvedData.name,
    status_uname: statusData.name,
    status_color: statusData.color || 'default',
    update_time_resolved: dealKfTime(order.update_time, isHistory),
    price_precision: pricePrecision,
    limit_price_resolved: dealKfPrice(order.limit_price, pricePrecision),
  };
};

export const dealOrderTrigger = (
  watcher: KungfuApi.Watcher,
  order: KungfuApi.OrderTrigger,
  isHistory = false,
  pricePrecision = 4,
  index,
): KungfuApi.OrderTriggerResolved => {
  const sourceResolvedData = resolveAccountId(
    watcher,
    order.source,
    order.dest,
  );
  const destResolvedData = resolveClientId(watcher, order.dest);
  const statusData = dealOrderTriggerStatus(order.status);
  return {
    ...order,
    source: order.source,
    dest: order.dest,
    uid_key: order.uid_key,
    source_resolved_data: sourceResolvedData,
    dest_resolved_data: destResolvedData,
    source_uname: sourceResolvedData.name,
    dest_uname: destResolvedData.name,
    status_uname: statusData.name || '--',
    status_color: statusData.color || 'default',
    update_time_resolved: dealKfTime(order.update_time, isHistory),
    insert_time_resolved:
      order.dest === 0 ? '--' : dealKfTime(order.insert_time, isHistory),
    price_precision: pricePrecision,
    limit_price_resolved: dealKfPrice(order.limit_price, pricePrecision),
    time_condition_resolved: dealTimeCondition(order.time_condition)
      ? dealTimeCondition(order.time_condition).name
      : '--',
    key: index + 1,
    action_flag_uname: dealTOrderTriggerFlag(order.action_flag).name,
  };
};

export const dealTrade = (
  watcher: KungfuApi.Watcher,
  trade: KungfuApi.Trade,
  orderStats: KungfuApi.DataTable<KungfuApi.OrderStat>,
  isHistory = false,
  pricePrecision = 4,
): KungfuApi.TradeResolved => {
  const sourceResolvedData = resolveAccountId(
    watcher,
    trade.source,
    trade.dest,
  );
  const destResolvedData = resolveClientId(watcher, trade.dest);
  const orderUKey = trade.order_id.toString(16).padStart(16, '0');
  const latencyData = dealOrderStat(orderStats, orderUKey) || {
    latencyTrade: '--',
    trade_time: BigInt(0),
  };
  return {
    ...trade,
    source: trade.source,
    dest: trade.dest,
    uid_key: trade.uid_key,
    source_resolved_data: sourceResolvedData,
    dest_resolved_data: destResolvedData,
    source_uname: sourceResolvedData.name,
    dest_uname: destResolvedData.name,
    trade_time_resolved: dealKfTime(trade.trade_time, isHistory),
    kf_time_resovlved: dealKfTime(latencyData.trade_time, isHistory),
    latency_trade: latencyData.latencyTrade,
    price_precision: pricePrecision,
    price_resolved: dealKfPrice(trade.price, pricePrecision),
  };
};

export const getPosClosableVolume = (position: KungfuApi.Position): bigint => {
  return isShotable(position.instrument_type) ||
    isT0(position.instrument_type, position.exchange_id)
    ? BigInt(Math.max(+Number(position.volume - position.frozen_total), 0))
    : BigInt(
        Math.max(+Number(position.yesterday_volume - position.frozen_total), 0),
      );
};

export const dealPosition = (
  watcher: KungfuApi.Watcher,
  pos: KungfuApi.Position,
  pricePrecision = 4,
): KungfuApi.PositionResolved => {
  const account_id_resolved = getIdByKfLocation(
    watcher.getLocation(pos.source_id),
  );
  const closable_volume = getPosClosableVolume(pos);
  const ukey = hashInstrumentUKey(pos.instrument_id, pos.exchange_id);
  const currency =
    ((watcher.ledger.Instrument[ukey] as KungfuApi.Instrument) || null)
      ?.currency || CurrencyEnum.Unknown;
  return {
    ...pos,
    currency,
    closable_volume,
    uid_key: pos.uid_key, // 隐式属性，...pos 并不能结构
    account_id_resolved,
    instrument_id_resolved: `${pos.instrument_id} ${
      ExchangeIds[pos.exchange_id]?.name ?? ''
    }`,
    price_precision: pricePrecision,
    last_price_resolved: dealKfPrice(pos.last_price, pricePrecision),
    avg_open_price_resolved: dealKfPrice(pos.avg_open_price, pricePrecision),
    unrealized_pnl_resolved: pos.avg_open_price
      ? dealAssetPrice(pos.unrealized_pnl, pricePrecision)
      : '--',
    open_volume: pos.open_volume ?? 0,
    static_yesterday: pos.static_yesterday ?? 0,
    close_volume:
      Number(pos.open_volume) +
        Number(pos.static_yesterday) -
        Number(pos.volume) || 0,
  };
};

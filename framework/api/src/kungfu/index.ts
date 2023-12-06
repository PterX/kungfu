import dayjs from 'dayjs';
import dayjsBusinessDays from 'dayjs-business-days';
import { kungfu } from '@kungfu-trader/kungfu-core';
import { kfLogger } from '@kungfu-trader/kungfu-js-api/utils/logUtils';
import { KF_RUNTIME_DIR } from '../config/pathConfig';
dayjs.extend(dayjsBusinessDays);

export const kf = kungfu();

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

export const dealKfTime = (nano: bigint, date = false): string => {
  if (nano === BigInt(0)) {
    return '--';
  }

  if (date) {
    return kf.formatTime(nano, '%m/%d %H:%M:%S.%N').slice(0, 18);
  }
  return kf.formatTime(nano, '%H:%M:%S.%N').slice(0, 12);
};

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
  const holderLocation = watcher.getLocation(pos.holder_uid);
  const account_id_resolved =
    pos.ledger_category === LedgerCategoryEnum.td
      ? `${holderLocation.group}_${holderLocation.name}`
      : '--';
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
        Number(pos.volume) ?? 0,
  };
};

export const promiseWithCachedPause = <T>(
  watcher: KungfuApi.Watcher,
  promiseFunc: () => Promise<T>,
  delay = 200,
): Promise<T> => {
  return new Promise((resolve, reject) => {
    const cachedLocation = {
      category: 'system',
      group: 'service',
      mode: 'live',
      name: 'cached',
    };

    let keyCachedPause = 10253;
    let keyCachedResume = 10254;
    for (const key in longfist.msgTypes) {
      const item = longfist.msgTypes[key];
      if (item === 'CachedPause') {
        keyCachedPause = Number(key);
      } else if (item === 'CachedResume') {
        keyCachedResume = Number(key);
      }
    }

    watcher.issueMark(keyCachedPause, cachedLocation);
    setTimeout(() => {
      promiseFunc()
        .then((res) => {
          resolve(res);
        })
        .catch((err) => {
          reject(err);
        })
        .finally(() => {
          watcher.issueMark(keyCachedResume, cachedLocation);
        });
    }, delay);
  });
};

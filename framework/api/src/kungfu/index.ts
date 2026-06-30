import dayjs from 'dayjs';
import dayjsBusinessDays from 'dayjs-business-days';
import { kungfu } from '@kungfu-tech/core';
import { kfLogger } from '@kungfu-tech/api/utils/logUtils';
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

export const hashSingleUKey = (arg: string | number | bigint) => {
  const uKey = `${arg}`;
  if (!ukeyCacheMap.has(uKey))
    ukeyCacheMap.set(uKey, arg.toString(16).padStart(16, '0'));

  return ukeyCacheMap.get(uKey) || '';
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

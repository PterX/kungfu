import { KfModeTypes } from '@kungfu-trader/kungfu-js-api/typings/enums';
import { getProcessIdByKfLocation } from '@kungfu-trader/kungfu-js-api/utils/commonUtils';

export const getMasterLocation = (
  mode: KfModeTypes = 'live',
): KungfuApi.KfLocation => {
  return {
    category: 'system',
    group: 'master',
    name: 'master',
    mode,
  };
};

export const getLedgerLocation = (
  mode: KfModeTypes = 'live',
): KungfuApi.KfLocation => {
  return {
    category: 'system',
    group: 'service',
    name: 'ledger',
    mode,
  };
};

export const getArhciveLocation = (
  mode: KfModeTypes = 'live',
): KungfuApi.KfLocation => {
  return {
    category: 'system',
    group: 'service',
    name: 'archive',
    mode,
  };
};

export const getDzxyLocation = (
  mode: KfModeTypes = 'live',
): KungfuApi.KfLocation => {
  return {
    category: 'system',
    group: 'service',
    name: 'dzxy',
    mode,
  };
};

export const getMasterProcessId = (mode: KfModeTypes = 'live') => {
  return getProcessIdByKfLocation(getMasterLocation(mode));
};

export const getLedgerProcessId = (mode: KfModeTypes = 'live') => {
  return getProcessIdByKfLocation(getLedgerLocation(mode));
};

export const getArchiveProcessId = (mode: KfModeTypes = 'live') => {
  return getProcessIdByKfLocation(getArhciveLocation(mode));
};

export const getDzxyProcessId = (mode: KfModeTypes = 'live') => {
  return getProcessIdByKfLocation(getDzxyLocation(mode));
};

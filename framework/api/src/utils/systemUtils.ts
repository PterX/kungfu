import { KfModeTypes } from '@kungfu-tech/api/typings/enums';
import { getProcessIdByKfLocation } from '@kungfu-tech/api/utils/commonUtils';

export const buildMasterLocation = (
  mode: KfModeTypes = 'live',
): KungfuApi.KfLocation => {
  return {
    category: 'system',
    group: 'master',
    name: 'master',
    mode,
  };
};

export const buildLedgerLocation = (
  mode: KfModeTypes = 'live',
): KungfuApi.KfLocation => {
  return {
    category: 'system',
    group: 'service',
    name: 'ledger',
    mode,
  };
};

export const buildCachedLocation = (
  mode: KfModeTypes = 'live',
): KungfuApi.KfLocation => {
  return {
    category: 'system',
    group: 'service',
    name: 'cached',
    mode,
  };
};

export const buildArchiveLocation = (
  mode: KfModeTypes = 'live',
): KungfuApi.KfLocation => {
  return {
    category: 'system',
    group: 'service',
    name: 'archive',
    mode,
  };
};

export const buildDzxyLocation = (
  mode: KfModeTypes = 'live',
): KungfuApi.KfLocation => {
  return {
    category: 'system',
    group: 'service',
    name: 'dzxy',
    mode,
  };
};

export const buildMasterProcessId = (mode: KfModeTypes = 'live') => {
  return getProcessIdByKfLocation(buildMasterLocation(mode));
};

export const buildLedgerProcessId = (mode: KfModeTypes = 'live') => {
  return getProcessIdByKfLocation(buildLedgerLocation(mode));
};

export const buildCachedProcessId = (mode: KfModeTypes = 'live') => {
  return getProcessIdByKfLocation(buildCachedLocation(mode));
};

export const buildArchiveProcessId = (mode: KfModeTypes = 'live') => {
  return getProcessIdByKfLocation(buildArchiveLocation(mode));
};

export const buildDzxyProcessId = (mode: KfModeTypes = 'live') => {
  return getProcessIdByKfLocation(buildDzxyLocation(mode));
};

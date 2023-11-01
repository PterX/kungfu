import path from 'path';
import fse from 'fs-extra';
import { configStore } from '../kungfu';
import {
  getResultUntilValuable,
  hidePasswordByLogger,
} from '../utils/commonUtils';
import { promiseWithCachedPause } from '../utils/tradingUtils';
import { kfLogger } from '../utils/logUtils';
import { BASE_DB_DIR } from '../config/pathConfig';

const buildConfigOperationPromise = <T>(
  originGetter: () => T,
  watcher?: KungfuApi.Watcher,
) =>
  watcher
    ? promiseWithCachedPause(watcher, () =>
        getResultUntilValuable(originGetter),
      )
    : getResultUntilValuable(originGetter);

export const getKfAllConfig = (
  watcher?: KungfuApi.Watcher,
): Promise<KungfuApi.KfConfigOrigin[]> => {
  if (fse.pathExistsSync(path.join(BASE_DB_DIR, 'config.db'))) {
    const promise = buildConfigOperationPromise(
      () => configStore.getAllConfig(),
      watcher,
    );
    return promise.then((allConfigs) => Object.values(allConfigs));
  } else {
    return Promise.resolve([]);
  }
};

export const setKfConfig = (
  kfLocation: KungfuApi.KfLocation,
  configValue: string,
  watcher?: KungfuApi.Watcher,
): Promise<boolean> => {
  const configForLog = hidePasswordByLogger(configValue);
  kfLogger.info(
    `Set Kungfu Config ${kfLocation.category} ${kfLocation.group} ${kfLocation.name} ${kfLocation.mode} ${configForLog}`,
  );
  const promise = buildConfigOperationPromise(
    () =>
      configStore.setConfig(
        kfLocation.category,
        kfLocation.group,
        kfLocation.name,
        kfLocation.mode,
        configValue,
      ),
    watcher,
  );
  return promise;
};

export const removeKfConfig = (
  kfLocation: KungfuApi.KfLocation,
  watcher?: KungfuApi.Watcher,
): Promise<boolean> => {
  kfLogger.info(
    `Remove Kungfu Config ${kfLocation.category} ${kfLocation.group} ${kfLocation.name}`,
  );
  const promise = buildConfigOperationPromise(
    () =>
      configStore.removeConfig(
        kfLocation.category,
        kfLocation.group,
        kfLocation.name,
        kfLocation.mode,
      ),
    watcher,
  );
  return promise;
};

export const getKfConfig = (
  strategyId: string,
  watcher?: KungfuApi.Watcher,
) => {
  const kfLocation: KungfuApi.KfLocation = getStrategyKfLocation(strategyId);
  const promise = buildConfigOperationPromise(
    () =>
      configStore.getConfig(
        kfLocation.category,
        kfLocation.group,
        kfLocation.name,
        kfLocation.mode,
      ),
    watcher,
  );
  return promise;
};

export const getStrategyKfLocation = (strategyId: string) => {
  return {
    category: 'strategy',
    group: 'default',
    name: strategyId,
    mode: 'live',
  };
};

import path from 'path';
import fse from 'fs-extra';
import { riskSettingStore, longfist } from '.';
import { getResultUntilValuable } from '../utils/commonUtils';
import { promiseWithCachedPause } from '../utils/tradingUtils';
import { kfLogger } from '../utils/logUtils';
import { BASE_DB_DIR } from '../config/pathConfig';

export const getAllKfRiskSettings = (
  watcher: KungfuApi.Watcher,
): Promise<KungfuApi.RiskSettingOrigin[]> => {
  kfLogger.info('Get kungfu RiskSettings');
  if (!fse.pathExistsSync(path.join(BASE_DB_DIR, 'config.db'))) {
    return Promise.resolve([]);
  }

  return promiseWithCachedPause(watcher, () => {
    return getResultUntilValuable(() =>
      riskSettingStore.getAllRiskSetting(),
    ).then((riskSettings) => Object.values(riskSettings));
  });
};

export const setAllKfRiskSettings = (
  watcher: KungfuApi.Watcher,
  riskSettings: KungfuApi.RiskSettingForSave[],
): Promise<boolean> => {
  kfLogger.info('Set kungfu RiskSettings');
  const kfRiskSetting = longfist.types.RiskSetting();
  const riskSettingResolved = riskSettings
    .filter((item) => item.category === 'td' && item.group && item.name)
    .map((item) => ({
      ...kfRiskSetting,
      ...item,
    }));

  return promiseWithCachedPause(watcher, () => {
    return getResultUntilValuable(() =>
      riskSettingStore.setAllRiskSetting(riskSettingResolved),
    );
  });
};

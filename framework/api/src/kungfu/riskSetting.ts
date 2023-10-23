import path from 'path';
import fse from 'fs-extra';
import { riskSettingStore, longfist, promiseWithDatabasePause } from '.';
import { kfLogger, getResultUntilValuable } from '../utils/busiUtils';
import { BASE_DB_DIR } from '../config/pathConfig';

export const getAllKfRiskSettings = (
  watcher: KungfuApi.Watcher,
): Promise<KungfuApi.RiskSettingOrigin[]> => {
  kfLogger.info('Get kungfu RiskSettings');
  if (!fse.pathExistsSync(path.join(BASE_DB_DIR, 'config.db'))) {
    return Promise.resolve([]);
  }

  return promiseWithDatabasePause(watcher, () => {
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

  return promiseWithDatabasePause(watcher, () => {
    return getResultUntilValuable(() =>
      riskSettingStore.setAllRiskSetting(riskSettingResolved),
    );
  });
};

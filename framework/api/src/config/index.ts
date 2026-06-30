import fse, { ensureFileSync } from 'fs-extra';
import {
  KF_CONFIG_DEFAULT_PATH,
  KF_CONFIG_PATH,
  KF_INSTRUMENTS_DEFAULT_PATH,
  KF_INSTRUMENTS_PATH,
} from './pathConfig';
import { readRootPackageJsonSync } from '@kungfu-tech/api/utils/fileUtils';
import {
  mergeObject,
  booleanProcessEnv,
} from '@kungfu-tech/api/utils/commonUtils';
import { getGlobalStorage } from '@kungfu-tech/api/utils/globalStorage';
const globalStorage = getGlobalStorage();

export const initKfConfig = () => {
  if (!fse.existsSync(KF_CONFIG_PATH)) {
    ensureFileSync(KF_CONFIG_PATH);
    let kfConfigJSON = fse.readJsonSync(KF_CONFIG_DEFAULT_PATH);
    const kfConfigInitValue =
      readRootPackageJsonSync()?.appConfig?.kfConfigInitValue;
    if (kfConfigInitValue && typeof kfConfigInitValue === 'object') {
      kfConfigJSON = mergeObject(kfConfigJSON, kfConfigInitValue);
    }

    if (
      globalStorage.getItem('isKungfuFirstRunning') &&
      !booleanProcessEnv(process.env.IF_CPUS_NUM_SAFE)
    ) {
      if (!kfConfigJSON.performance) kfConfigJSON.performance = {};
      kfConfigJSON.performance.bypassAccounting = true;
      kfConfigJSON.performance.bypassRefreshBook = true;
    }

    fse.outputJsonSync(KF_CONFIG_PATH, kfConfigJSON);
  }
};

export const initKfDefaultInstruments = () => {
  if (!fse.existsSync(KF_INSTRUMENTS_PATH)) {
    ensureFileSync(KF_INSTRUMENTS_PATH);
    const kfInstrumentsJSON = fse.readJsonSync(KF_INSTRUMENTS_DEFAULT_PATH);
    fse.outputJsonSync(KF_INSTRUMENTS_PATH, kfInstrumentsJSON);
  }
};

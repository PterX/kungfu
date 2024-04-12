import { getRendererProcessId, kf } from './index';
import { KF_RUNTIME_DIR } from '@kungfu-trader/kungfu-js-api/config/pathConfig';
import { getKfGlobalSettingsValue } from '@kungfu-trader/kungfu-js-api/config/globalSettings';
import { kfLogger } from '@kungfu-trader/kungfu-js-api/utils/logUtils';
import { booleanProcessEnv } from '@kungfu-trader/kungfu-js-api/utils/commonUtils';

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
    process.env.MILLISECONDS_SLEEP_AFTER_STEP ?? 100;

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

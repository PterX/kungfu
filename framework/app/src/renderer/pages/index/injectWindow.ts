import workers from '@kungfu-trader/kungfu-app/src/renderer/assets/workers';
import { watcher } from '@kungfu-trader/kungfu-js-api/kungfu/watcher';
import {
  basketInstrumentStore,
  basketStore,
  commissionStore,
  configStore,
  sessionStore,
  getRendererProcessId,
  io,
  kf,
  riskSettingStore,
} from '@kungfu-trader/kungfu-js-api/kungfu';
import { KF_RUNTIME_DIR } from '@kungfu-trader/kungfu-js-api/config/pathConfig';

window.workers = workers;
window.watcher = watcher;
window.kungfu = kf;
window.basketStore = basketStore;
window.basketInstrumentStore = basketInstrumentStore;
window.configStore = configStore;
window.riskSettingStore = riskSettingStore;
window.commissionStore = commissionStore;
window.sessionStore = sessionStore;
window.io = io;

window.testCase = {};
window.testCase.crashTheWatcher = () => {
  const id = getRendererProcessId();
  return kf.watcher(KF_RUNTIME_DIR, kf.formatStringToHashHex(id), false, false);
};

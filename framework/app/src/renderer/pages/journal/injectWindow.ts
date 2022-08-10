import workers from '@kungfu-trader/kungfu-app/src/renderer/assets/workers';
import {
  getWatcherId,
  watcher,
} from '@kungfu-trader/kungfu-js-api/kungfu/watcher';
import { assemble, kf } from '@kungfu-trader/kungfu-js-api/kungfu';
import { KF_RUNTIME_DIR } from '@kungfu-trader/kungfu-js-api/config/pathConfig';

window.workers = workers;
window.watcher = watcher;
window.assemble = assemble;
window.kungfu = kf;

window.testCase = {};
window.testCase.crashTheWatcher = () => {
  const id = getWatcherId();
  return kf.watcher(KF_RUNTIME_DIR, kf.formatStringToHashHex(id), false, false);
};

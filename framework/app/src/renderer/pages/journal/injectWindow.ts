// import workers from './workers';

// window.workers = workers;
import { KfCategoryTypes } from '@kungfu-trader/kungfu-js-api/typings/enums';
import { useGlobalStore } from '@kungfu-trader/kungfu-app/src/renderer/pages/index/store/global';

window.testCase = {};
window.testCase.enableReplay = (
  key: KfCategoryTypes | 'ledger',
  value: boolean,
) => {
  useGlobalStore().setTestCase(key, value);
};

window.testCase.enableTdReplay = () => {
  window.testCase.enableReplay('td', true);
};

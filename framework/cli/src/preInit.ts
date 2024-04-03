import './injectGlobal';
import {
  initKfConfig,
  initKfDefaultInstruments,
} from '@kungfu-trader/kungfu-js-api/config';
import { getGlobalStorage } from '@kungfu-trader/kungfu-js-api/utils/globalStorage';

const globalStorage = getGlobalStorage();

initKfConfig();
initKfDefaultInstruments();

globalStorage.setItem('isKungfuFirstRunning', false);

import './injectGlobal';
import {
  initKfConfig,
  initKfDefaultInstruments,
} from '@kungfu-trader/kungfu-js-api/config';
import globalStorage from '@kungfu-trader/kungfu-js-api/utils/globalStorage';

initKfConfig();
initKfDefaultInstruments();

globalStorage.setItem('ifNotFirstRunning', true);

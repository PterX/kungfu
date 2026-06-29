import './injectGlobal';
import {
  initKfConfig,
  initKfDefaultInstruments,
} from '@kungfu-tech/api/config';
import { getGlobalStorage } from '@kungfu-tech/api/utils/globalStorage';

const globalStorage = getGlobalStorage();

initKfConfig();
initKfDefaultInstruments();

globalStorage.setItem('isKungfuFirstRunning', false);

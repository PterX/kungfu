import '@kungfu-tech/api/hooks';
import pm2 from '@kungfu-tech/api/utils/pm2Custom';
import globalBus from '@kungfu-tech/api/utils/globalBus';
globalThis.pm2 = pm2;
globalThis.globalBus = globalBus;

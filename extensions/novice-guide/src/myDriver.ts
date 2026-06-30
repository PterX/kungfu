import { DriveStep } from 'driver.js';

import VueI18n from '@kungfu-tech/api/language';
import { waitElementMounted } from './utils/driver';

import {
  buildAddLocationSteps,
  buildDriverStep,
  buildCenterModalStep,
  buildWaitClickStep,
  generateDriverConfig,
  generateDriver,
} from './utils/driver';

const { t } = VueI18n.global;

const defaultDriverConfigs = generateDriverConfig();

export const myDriver = generateDriver('default', defaultDriverConfigs);

const addLocationsSteps: DriveStep[] = [
  ...buildAddLocationSteps(myDriver, 'Td', true, true, () => {
    waitElementMounted('.kf-message').then((el) => {
      el.remove();
      myDriver.moveNext();
    });
  }),
  ...buildAddLocationSteps(myDriver, 'Md'),
  ...buildAddLocationSteps(myDriver, 'Strategy'),
  buildDriverStep(
    myDriver,
    `#Order .kf-dashboard__body`,
    t('noviceGuide.showOrder'),
    t('noviceGuide.showOrderDesc'),
  ),
  buildWaitClickStep(
    myDriver,
    `#Strategy .kf-dashboard__body .ant-table-body tbody tr:nth-child(2) button.ant-switch`,
    t('noviceGuide.stopStrategy'),
    t('noviceGuide.stopStrategyDesc'),
  ),
];

const allSteps: DriveStep[] = [
  buildCenterModalStep(
    myDriver,
    t('noviceGuide.welcome'),
    t('noviceGuide.welcomeDesc'),
    {
      popover: {
        nextBtnText: t('noviceGuide.start'),
      },
    },
  ),
  ...addLocationsSteps,
  buildCenterModalStep(
    myDriver,
    t('noviceGuide.finish'),
    t('noviceGuide.finishDesc'),
  ),
];

myDriver.setSteps(allSteps);

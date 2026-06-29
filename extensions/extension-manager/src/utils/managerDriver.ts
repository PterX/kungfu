import { DriveStep } from 'driver.js';
import { onMounted, getCurrentInstance, onBeforeUnmount } from 'vue';
import { storeToRefs } from 'pinia';
import { Subscription } from 'rxjs';

import VueI18n from '@kungfu-tech/api/language';
import { LoginAuthingKeys } from '@kungfu-tech/kfx-ui-login-authing/src/configs';
import {
  generateDriver,
  generateDriverConfig,
  buildDriverStep,
} from '@kungfu-tech/kfx-ui-novice-guide/src/utils/driver';
import { useAuthingCredential } from '@kungfu-tech/kfx-ui-login-authing/src/utils/externalUtils';

import { useExtManagerStore } from '../store';
import { AuthingAccessKey } from '../configs';

const { t } = VueI18n.global;

export const useManagerDriver = () => {
  const app = getCurrentInstance();
  const extManagerStore = useExtManagerStore();
  const { currentExtension } = storeToRefs(extManagerStore);
  const { checkCurrentAccountAccess } = useAuthingCredential();

  const defaultDriverConfigs = generateDriverConfig();
  const subs: Array<Subscription | undefined> = [];

  const managerDriver = generateDriver(
    'extension-manager',
    defaultDriverConfigs,
  );

  const managerDriverSteps: DriveStep[] = [
    buildDriverStep(
      managerDriver,
      '.kf-authing-login-modal',
      t('extensionManager.guide.login'),
      t('extensionManager.guide.loginDesc'),
      {
        popover: {
          showButtons: ['close'],
        },
        onHighlighted: () => {
          const sub = app?.proxy?.$globalBus.subscribe((data) => {
            if (data.tag === LoginAuthingKeys.LoggedIn) {
              if (currentExtension.value) {
                const access = checkCurrentAccountAccess(
                  currentExtension.value.access[AuthingAccessKey],
                );
                if (!access) {
                  managerDriver.moveNext();
                } else {
                  managerDriver.destroy();
                  extManagerStore.setIsInExtUse(true);
                }
              }
              sub?.unsubscribe();
            }
          });
          subs.push(sub);
        },
      },
    ),
    buildDriverStep(
      managerDriver,
      '#kf-ext-unusable-button',
      t('extensionManager.guide.toGetAccess'),
      t('extensionManager.guide.toGetAccessDesc', {
        extName: currentExtension.value?.name || '',
      }),
    ),
    buildDriverStep(
      managerDriver,
      '.kf-authing-btn__warp',
      t('extensionManager.guide.toSyncData'),
      t('extensionManager.guide.toSyncDataDesc'),
      {
        popover: {
          side: 'top',
        },
      },
    ),
  ];

  managerDriver.setSteps(managerDriverSteps);

  onMounted(() => {
    const sub = app?.proxy?.$globalBus.subscribe((data) => {
      if (data.tag === '') {
        managerDriver.moveNext();
      }
    });
    subs.push(sub);
  });

  onBeforeUnmount(() => {
    subs.forEach((sub) => sub?.unsubscribe());
  });

  return {
    managerDriver,
  };
};

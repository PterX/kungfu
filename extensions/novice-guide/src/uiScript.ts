import path from 'path';
import { watchEffect, App, h } from 'vue';
import { setKfConfig } from '@kungfu-tech/api/kungfu/store';
import { kfLogger } from '@kungfu-tech/api/utils/logUtils';
import { LifeCycleKeys } from '@kungfu-tech/api/hooks/lifeCycleHook';
import { KfHookKeeper } from '@kungfu-tech/api/hooks';
import { openReadmeModal } from '@kungfu-tech/gui/src/renderer/assets/methods/uiUtils';
import ResolveExtConfig from '@kungfu-tech/api/hooks/resolveExtConfigHook';
import VueI18n from '@kungfu-tech/api/language';

import { STRATEGY_DEMO_DIR } from './configs';

import {
  DefaultAutoAddConfigs,
  getStrategyDemoAutoAddConfigs,
} from './configs';
import { myDriver } from './myDriver';
const { t } = VueI18n.global;

export default {
  install: async (Vue: App<Element>) => {
    const strategyDemoConfigs = await getStrategyDemoAutoAddConfigs();

    const resolveExtConfigHooks = globalThis.HookKeeper.getHooks()
      .resolveExtConfig as typeof ResolveExtConfig;

    resolveExtConfigHooks.register(
      { category: 'strategy', group: 'default', name: '*', mode: '*' },
      (_: KungfuApi.KfLocation, extConfig: KungfuApi.KfExtConfig) => {
        return {
          ...extConfig,
          settings: (extConfig.settings || []).map((setting) => {
            if (setting.type === 'file') {
              return {
                ...setting,
                defaultDir: STRATEGY_DEMO_DIR,
              };
            }
            return setting;
          }),
        };
      },
    );
    kfLogger.info('noviceGuideCache', myDriver.hasRan);
    if (myDriver.hasRan) return;
    myDriver.setHasRan(true);

    (globalThis.HookKeeper as KfHookKeeper)
      .getHooks()
      .lifeCycle.register(
        LifeCycleKeys.BeforeAppMount,
        'novice-guide',
        async () => {
          try {
            const { watcher } = window;

            const configs = [...DefaultAutoAddConfigs, ...strategyDemoConfigs];
            for (const config of configs) {
              await setKfConfig(
                config.location,
                JSON.stringify({
                  ...(config.initValue ?? {}),
                  add_time: +Date.now() * Math.pow(10, 6),
                }),
                watcher,
              );
            }
          } catch (error) {
            kfLogger.error(error);
          }
        },
      );

    const watchStartGuide = () => {
      const stop = watchEffect(() => {
        if (Vue.config.globalProperties.$route.path === '/main') {
          setTimeout(() => {
            myDriver.drive(0, true);
            stop && stop();
          }, 200);
        }
      });
    };

    const sub = Vue.config.globalProperties.$globalBus.subscribe((data) => {
      if (
        data.tag === 'processStatus' &&
        data.name === 'extraResourcesLoading' &&
        data.status === 'online'
      ) {
        sub.unsubscribe();

        const privacyPolicyPath = path.join(
          __dirname,
          'kungfu-privacy-policy.md',
        );
        openReadmeModal(privacyPolicyPath, {
          width: '40vw',
          closable: false,
          keyboard: false,
          icon: () => h('div'),
          okText: t('noviceGuide.agree'),
          onOk() {
            watchStartGuide();
          },
        });
      }
    });
  },
};

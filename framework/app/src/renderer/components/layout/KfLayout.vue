<script setup lang="ts">
import {
  toRaw,
  watchEffect,
  computed,
  getCurrentInstance,
  onBeforeUnmount,
  nextTick,
  ref,
} from 'vue';
import { useRouter } from 'vue-router';
import { SlidersOutlined, SettingOutlined } from '@ant-design/icons-vue';
import KfProcessStatusController from '@kungfu-trader/kungfu-app/src/renderer/components/layout/KfProcessStatusController.vue';
import KfUpdateController from '@kungfu-trader/kungfu-app/src/renderer/components/layout/KfUpdateController.vue';
import { useExtConfigsRelated } from '../../assets/methods/actionsUtils';
import { isUpdateVersionLogicEnable } from '@kungfu-trader/kungfu-js-api/utils/busiUtils';
import globalBus from '@kungfu-trader/kungfu-js-api/utils/globalBus';
import KfGlobalSettingModal from '../public/KfGlobalSettingModal.vue';
import { useLanguage } from '@kungfu-trader/kungfu-js-api/language';
import {
  getLogoPath,
  isDefaultLogo,
} from '@kungfu-trader/kungfu-js-api/config/brand';
import VueI18n from '@kungfu-trader/kungfu-js-api/language';
import { readRootPackageJsonSync } from '@kungfu-trader/kungfu-js-api/utils/fileUtils';
const { t } = VueI18n.global;

const logoPath = isDefaultLogo()
  ? require('@kungfu-trader/kungfu-app/src/renderer/assets/svg/LOGO.svg')
  : getLogoPath();

const app = getCurrentInstance();
const router = useRouter();
const globalSettingModalVisible = ref<boolean>(false);
const menuSelectedKeys = ref<string[]>(['main']);

const { uiExtConfigs } = useExtConfigsRelated();
const { isLanguageKeyAvailable } = useLanguage();

const isExtSidebarShow = ref<Record<string, boolean>>({});

const sidebarFooterComponentConfigs = computed(() => {
  return Object.keys(uiExtConfigs.value)
    .filter((key) => uiExtConfigs.value[key].position === 'sidebar_footer')
    .map((key) => {
      return {
        ...uiExtConfigs.value[key],
        key,
      };
    });
});

const footerComponentConfigs = computed(() => {
  return Object.keys(uiExtConfigs.value)
    .filter((key) => uiExtConfigs.value[key].position === 'footer')
    .map((key) => {
      return {
        ...uiExtConfigs.value[key],
        key,
      };
    });
});

const sidebarComponentConfigs = computed(() => {
  const rootPackageJson = readRootPackageJsonSync();
  const customSidebar = rootPackageJson.appConfig?.customSidebar ?? {};
  const uiExtConfigsWithMain = toRaw(uiExtConfigs.value);
  uiExtConfigsWithMain['main'] = {
    access: {},
    assets: {},
    category: 'ui',
    components: null,
    dependencies: {},
    description: '',
    exhibit: {} as KungfuApi.KfExhibitConfig,
    extPath: '',
    key: 'main',
    keepAlive: true,
    name: t('baseConfig.main_panel'),
    position: 'sidebar',
    sidebarIndex: 0,
    readmePath: '',
    releaseNotePath: '',
    script: '',
    silent: false,
    version: '',
  };

  return Object.keys(uiExtConfigsWithMain)
    .filter((key) => uiExtConfigsWithMain[key].position === 'sidebar')
    .map((key) => {
      if (customSidebar[key]) {
        const { sidebarIndex, name } = customSidebar[key];
        uiExtConfigsWithMain[key].sidebarIndex = sidebarIndex ?? -1;
        uiExtConfigsWithMain[key].name = name ?? uiExtConfigsWithMain[key].name;
      }
      return {
        ...uiExtConfigsWithMain[key],
        key,
      };
    })
    .sort((a, b) => {
      return (b.sidebarIndex || 0) - (a.sidebarIndex || 0);
    });
});

const stop = watchEffect(() => {
  const firstSidebarComponent = sidebarComponentConfigs.value[0];
  if (firstSidebarComponent && !router.hasRoute('default')) {
    router.addRoute({
      path: '/',
      name: 'default',
      redirect: `/${firstSidebarComponent.key}`,
    });
    router.push({ path: `/${firstSidebarComponent.key}` });
    menuSelectedKeys.value = [firstSidebarComponent.key];

    nextTick(() => {
      stop();
    });
  }
});

const busSubscription = globalBus.subscribe((data: KfEvent.KfBusEvent) => {
  if (data.tag === 'main') {
    switch (data.name) {
      case 'open-setting-dialog':
        globalSettingModalVisible.value = true;
    }
  }

  if (data.tag === 'switch-sidebar') {
    const targetKey = data.targetKey || '';
    const isInSidebar = sidebarComponentConfigs.value.some(
      (item) => item.key === targetKey,
    );
    if (isInSidebar) {
      menuSelectedKeys.value = [targetKey];
      handleToPage(`/${targetKey}`);
    }
  }

  if (data.tag === 'show-or-hide-extension-sidebar') {
    isExtSidebarShow.value[data.key || ''] = data.target;
    if (data.target === false) {
      menuSelectedKeys.value = ['main'];
      handleToPage('/main');
    }
  }
});

onBeforeUnmount(() => {
  busSubscription.unsubscribe();
});

function handleToPage(pathname: string) {
  if (app?.proxy) {
    app.proxy.$router.push(pathname);
  }
}
</script>
<template>
  <a-layout class="kf-layout">
    <a-layout>
      <a-layout-sider class="kf-layout-sider" width="64px">
        <div class="kf-header-logo">
          <img :src="logoPath" />
        </div>
        <a-menu
          v-model:selectedKeys="menuSelectedKeys"
          class="kf-layout-menu"
          mode="vertical"
          style="width: 64px"
        >
          <template v-for="config in sidebarComponentConfigs">
            <a-menu-item
              v-if="isExtSidebarShow[config.key] !== false"
              :key="config.key"
              @click="handleToPage(`/${config.key}`)"
            >
              <template #icon>
                <component
                  :is="config.key"
                  v-if="config.key !== 'main'"
                ></component>
                <sliders-outlined
                  v-if="config.key === 'main'"
                  style="font-size: 24px"
                />
              </template>
              <span>
                {{
                  isLanguageKeyAvailable(config.name)
                    ? $t(config.name)
                    : config.name
                }}
              </span>
            </a-menu-item>
          </template>
        </a-menu>
        <div class="kf-sidebar-footer__warp">
          <div
            v-for="config in sidebarFooterComponentConfigs"
            :key="config.key"
            class="kf-sidebar-footer-btn__warp"
            :title="
              isLanguageKeyAvailable(config.name)
                ? $t(config.name)
                : config.name
            "
          >
            <component :is="config.key"></component>
          </div>
          <div
            class="kf-sidebar-footer-btn__warp"
            @click="globalSettingModalVisible = true"
          >
            <setting-outlined class="kf-hover" style="font-size: 24px" />
          </div>
        </div>
      </a-layout-sider>
      <a-layout style="padding: 0px 8px 0 8px; box-sizing: border-box">
        <a-layout-content id="kf-layout-content" style="position: relative">
          <slot></slot>
        </a-layout-content>
      </a-layout>
    </a-layout>
    <a-layout-footer class="kf-layout-footer">
      <div class="kf-footer-box__warp">
        <KfProcessStatusController
          class="kf-footer-box__warp"
        ></KfProcessStatusController>
      </div>
      <div class="kf-footer-box__warp">
        <KfUpdateController
          v-if="isUpdateVersionLogicEnable()"
          class="kf-footer-box__warp"
        ></KfUpdateController>
      </div>
      <div
        v-for="config in footerComponentConfigs"
        :key="config.key"
        class="kf-footer-box__warp"
      >
        <component :is="config.key"></component>
      </div>
    </a-layout-footer>
    <KfGlobalSettingModal
      v-if="globalSettingModalVisible"
      v-model:visible="globalSettingModalVisible"
    ></KfGlobalSettingModal>
  </a-layout>
</template>

<style lang="less">
@import '@kungfu-trader/kungfu-app/src/renderer/assets/less/variables.less';

.kf-layout {
  height: 100%;

  .kf-layout-sider {
    .ant-layout-sider-children {
      display: flex;
      flex-direction: column;
      justify-content: space-between;
      position: relative;

      .kf-header-logo {
        width: 100%;
        height: @layout-side-width;
        background: @primary-color;
        position: relative;

        img {
          position: absolute;
          width: 100%;
          height: 100%;
          left: 0;
          top: 0;
        }
      }

      .kf-layout-menu.ant-menu-root.ant-menu-vertical {
        position: absolute;
        top: 40%;
        transform: translateY(-50%);
        left: 0;

        .ant-menu-item {
          height: @layout-side-width;
          line-height: @layout-side-width;
          display: flex;
          justify-content: space-evenly;
          align-items: center;
          flex-direction: column;
          padding: 0 8px;

          > span {
            display: block;
          }

          .ant-menu-title-content {
            height: auto;
            line-height: 1;
            margin-left: 0;
          }
        }
      }

      .kf-sidebar-footer__warp {
        .kf-sidebar-footer-btn__warp {
          width: 100%;
          height: 56px;
          line-height: 56px;
          text-align: center;

          .anticon {
            font-size: 24px;
            &:hover {
              color: @primary-color;
              cursor: pointer;
            }
          }
        }
      }
    }
  }

  .kf-layout-footer {
    height: @layout-footer-height;
    line-height: @layout-footer-height;
    padding: 0 8px 0 0;
    z-index: 1000;

    .kf-footer-box__warp {
      float: right;
      height: 100%;
      cursor: pointer;
      font-size: 12px;
      font-weight: bold;
      font-size: 12px;
      color: @primary-color;

      > div {
        padding: 0 8px;
      }

      &:hover {
        background: @item-active-bg;
      }
    }
  }
}
</style>

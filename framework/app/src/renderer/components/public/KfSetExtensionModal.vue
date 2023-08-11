<script setup lang="ts">
import { ref, onMounted, computed, getCurrentInstance } from 'vue';

import {
  getInstrumentTypeData,
  getStrategyExtTypeData,
  getExtConfigList,
  isTdMd,
  isOperator,
} from '@kungfu-trader/kungfu-js-api/utils/busiUtils';
import { useModalVisible } from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/uiUtils';
import {
  KfCategoryTypes,
  InstrumentTypes,
  StrategyExtTypes,
} from '@kungfu-trader/kungfu-js-api/typings/enums';
import { useExtConfigsRelated } from '../../assets/methods/actionsUtils';
import VueI18n, { useLanguage } from '@kungfu-trader/kungfu-js-api/language';
import { readRootPackageJsonSync } from '@kungfu-trader/kungfu-js-api/utils/fileUtils';

const { t } = VueI18n.global;

const props = withDefaults(
  defineProps<{
    visible: boolean;
    extensionType: KfCategoryTypes;
    extFilter?: (extConfig: KungfuApi.KfExtConfig) => boolean;
  }>(),
  {
    visible: false,
    extensionType: 'td',
    extFilter: () => true,
  },
);

defineEmits<{
  (e: 'confirm', sourceId: string): void;
  (e: 'update:visible', visible: boolean): void;
  (e: 'close'): void;
}>();

const app = getCurrentInstance();
const { extConfigs } = useExtConfigsRelated();
const selectedExtension = ref<string>('');
const availExtensionList = computed(() => {
  return getExtConfigList(extConfigs.value, props.extensionType).filter(
    (extConfig) => props.extFilter(extConfig),
  );
});

const { modalVisible, closeModal } = useModalVisible(props.visible);
const { isLanguageKeyAvailable } = useLanguage();

const modalTitle = computed(() => {
  switch (props.extensionType) {
    case 'td':
    case 'md':
      return t('select_broker_ext');
    case 'operator':
      return t('select_operator_ext');
    case 'strategy':
      return t('select_trade_task');
    default:
      return t('select_plugin_type');
  }
});

onMounted(() => {
  if (selectedExtension.value === '') {
    if (availExtensionList.value.length) {
      const packageJSON = readRootPackageJsonSync();
      let extension = '';
      if (packageJSON.appConfig?.defaultExtension) {
        extension =
          packageJSON.appConfig?.defaultExtension[props.extensionType];
      }
      for (let i = 0; i < availExtensionList.value.length; i++) {
        if (availExtensionList.value[i].key === extension) {
          selectedExtension.value = availExtensionList.value[i].key;
          return;
        }
      }
      selectedExtension.value = availExtensionList.value[0].key;
    }
  }
});

function handleConfirm() {
  app && app.emit('confirm', selectedExtension.value);
  closeModal();
}

function getKungfuTradeValueCommonDataByExtType(
  category: KfCategoryTypes,
  extType: InstrumentTypes | StrategyExtTypes,
): KungfuApi.KfTradeValueCommonData | null {
  if (isTdMd(category)) {
    return getInstrumentTypeData(extType as InstrumentTypes);
  }

  if (isOperator(category)) {
    return null;
  }

  return getStrategyExtTypeData(extType as StrategyExtTypes);
}
</script>
<template>
  <a-modal
    v-model:visible="modalVisible"
    class="kf-set-source-modal"
    :width="500"
    :title="modalTitle"
    :destroy-on-close="true"
    @cancel="closeModal"
    @ok="handleConfirm"
  >
    <a-radio-group v-model:value="selectedExtension">
      <a-radio
        v-for="item in availExtensionList"
        :key="item.key"
        :value="item.key"
        :style="{
          'min-height': '36px',
          'line-height': '36px',
          'font-size': '16px',
          'min-width': '45%',
        }"
      >
        <span class="source-name__txt">
          {{ isLanguageKeyAvailable(item.name) ? $t(item.name) : item.name }}
        </span>
        <span class="source-id__txt">{{ item.key }}</span>
        <template v-for="(extType, index) in item.type">
          <a-tag
            v-if="
              getKungfuTradeValueCommonDataByExtType(extensionType, extType)
            "
            :key="index"
            :color="
              getKungfuTradeValueCommonDataByExtType(extensionType, extType)
                ?.color || 'default'
            "
          >
            {{
              getKungfuTradeValueCommonDataByExtType(extensionType, extType)
                ?.name || ''
            }}
          </a-tag>
        </template>
      </a-radio>
    </a-radio-group>
  </a-modal>
</template>
<style lang="less">
.kf-set-source-modal {
  .source-name__txt {
    margin-right: 8px;
    font-weight: bold;
  }

  .source-id__txt {
    margin-right: 8px;
    font-size: 14px;
    color: @text-color-secondary;
  }
}
</style>

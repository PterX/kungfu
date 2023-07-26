<script setup lang="ts">
import { ref, onMounted, getCurrentInstance } from 'vue';
import { ArrowRightOutlined } from '@ant-design/icons-vue';

import { useModalVisible } from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/uiUtils';
import { useLanguage } from '@kungfu-trader/kungfu-js-api/language';
import VueI18n from '@kungfu-trader/kungfu-js-api/language';
import { FundTransTypeEnum } from '@kungfu-trader/kungfu-js-api/typings/enums';

const { t } = VueI18n.global;

const props = withDefaults(
  defineProps<{
    visible: boolean;
  }>(),
  {
    visible: false,
  },
);

defineEmits<{
  (e: 'confirm', tansType: FundTransTypeEnum): void;
  (e: 'update:visible', visible: boolean): void;
  (e: 'close'): void;
}>();

const app = getCurrentInstance();
const selectedTransType = ref<FundTransTypeEnum>(
  FundTransTypeEnum.BetweenNodes,
);
const availTransRecordList = ref([
  {
    key: FundTransTypeEnum.BetweenNodes,
    name: ['HTS', 'HTS'],
  },
  {
    key: FundTransTypeEnum.TrancIn,
    name: [t('fundTrans.centralized_counter'), 'HTS'],
  },
  {
    key: FundTransTypeEnum.TrancOut,
    name: ['HTS', t('fundTrans.centralized_counter')],
  },
]);

const { modalVisible, closeModal } = useModalVisible(props.visible);
const { isLanguageKeyAvailable } = useLanguage();

onMounted(() => {
  if (selectedTransType.value === 'between_nodes') {
    if (availTransRecordList.value.length) {
      selectedTransType.value = availTransRecordList.value[0].key;
    }
  }
});

function handleConfirm() {
  app && app.emit('confirm', selectedTransType.value);
  closeModal();
}
</script>
<template>
  <a-modal
    v-model:visible="modalVisible"
    class="kf-set-source-modal"
    :width="500"
    :title="$t('fundTrans.modal_title')"
    :destroy-on-close="true"
    @cancel="closeModal"
    @ok="handleConfirm"
  >
    <p
      :style="{
        'font-size': '16px',
      }"
    >
      {{ $t('fundTrans.trans_selection') }}
    </p>
    <a-radio-group v-model:value="selectedTransType">
      <a-radio
        v-for="item in availTransRecordList"
        :key="item.key"
        :value="item.key"
        :style="{
          'min-height': '36px',
          'line-height': '36px',
          'font-size': '16px',
          'min-width': '100%',
        }"
      >
        <span class="source-name__txt">
          {{
            isLanguageKeyAvailable(item.name[0])
              ? $t(item.name[0])
              : item.name[0]
          }}
        </span>
        <ArrowRightOutlined style="margin-right: 8px" />
        <span class="source-name__txt">
          {{
            isLanguageKeyAvailable(item.name[1])
              ? $t(item.name[1])
              : item.name[1]
          }}
        </span>
      </a-radio>
    </a-radio-group>
  </a-modal>
</template>
<style lang="less">
.kf-set-source-modal {
  .source-name__txt {
    margin-right: 8px;
  }

  .source-id__txt {
    margin-right: 8px;
    font-size: 14px;
    color: @text-color-secondary;
  }
}
</style>

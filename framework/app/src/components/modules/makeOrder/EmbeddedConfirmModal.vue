<script setup lang="ts">
import { useModalVisible } from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/uiUtils';
import { computed, reactive, ref, getCurrentInstance } from 'vue';
import { orderInputTrans } from './config';
import VueI18n from '@kungfu-trader/kungfu-js-api/language';
import {
  TimeConditionEnum,
  OrderTriggerParkedTypeEnum,
} from '@kungfu-trader/kungfu-js-api/typings/enums';

const { t } = VueI18n.global;
const app = getCurrentInstance();

const props = withDefaults(
  defineProps<{
    visible: boolean;
    embeddedOrderInput: Record<string, KungfuApi.KfTradeValueCommonData>;
  }>(),
  {
    visible: false,
  },
);

defineEmits<{
  (
    e: 'confirm',
    volumeList: {
      parked_type: OrderTriggerParkedTypeEnum;
      time_condition: TimeConditionEnum;
    },
  ): void;
  (e: 'update:visible', visible: boolean): void;
  (e: 'close'): void;
}>();

const { modalVisible, closeModal } = useModalVisible(props.visible);

const formState = reactive({
  parked_type: OrderTriggerParkedTypeEnum.Server,
  time_condition: TimeConditionEnum.GFA,
});
const serverEmbeddedRadio = ref([
  {
    label: t('tradingConfig.GFA'),
    value: TimeConditionEnum.GFA,
  },
]);

const formData = computed<Record<string, KungfuApi.KfTradeValueCommonData>>(
  () => {
    const transKeys = Object.keys(orderInputTrans);
    const results = Object.keys(props.embeddedOrderInput).reduce((pre, key) => {
      if (
        props.embeddedOrderInput[key].name !== '' &&
        key !== 'price_type' &&
        transKeys.includes(key)
      ) {
        pre[key] = props.embeddedOrderInput[key];
      }
      return pre;
    }, {});
    return results;
  },
);

function handleConfirm() {
  app && app.emit('confirm', formState);
  closeModal();
}
</script>

<template>
  <a-modal
    v-model:visible="modalVisible"
    class="kf-embedded-confirm-modal"
    :width="420"
    :title="$t('tradingConfig.embedded_order_title')"
    :destroy-on-close="true"
    @cancel="closeModal"
    @ok="handleConfirm"
  >
    <div class="embedded-content-wrap">
      <div class="embedded-order-type">
        <a-form
          ref="formRef"
          class="kf-config-form"
          :model="formState"
          :colon="false"
          :scroll-to-first-error="true"
        >
          <a-form-item>
            <a-radio-group v-model:value="formState.parked_type">
              <a-radio :value="OrderTriggerParkedTypeEnum.Server">
                {{ t('tradingConfig.server_embedded_label') }}
              </a-radio>
            </a-radio-group>
          </a-form-item>
          <a-form-item
            v-if="formState.parked_type === OrderTriggerParkedTypeEnum.Server"
            style="margin-left: 14px"
          >
            <a-radio-group v-model:value="formState.time_condition">
              <a-radio
                v-for="item in serverEmbeddedRadio"
                :key="item.label"
                :value="item.value"
              >
                <span>{{ item.label }}</span>
              </a-radio>
            </a-radio-group>
          </a-form-item>
        </a-form>
      </div>
      <div
        v-for="(item, key) in formData"
        :key="key"
        class="trading-data-detail-row"
      >
        <span class="label">{{ orderInputTrans[key] }}</span>
        <span class="value" :style="{ color: `${item.color}` }">
          {{ item.name }}
        </span>
      </div>
    </div>
  </a-modal>
</template>
<style lang="less">
.kf-embedded-confirm-modal {
  .ant-modal-body {
    padding: 24px;
  }
  .embedded-content-wrap {
    .trading-data-detail-row {
      .label,
      .value {
        display: inline-block;
        font-size: 14px;
      }

      .label {
        color: @text-color-secondary;
        padding-right: 8px;
      }
    }
  }
}
</style>

<script setup lang="ts">
import { useModalVisible } from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/uiUtils';
import { computed, getCurrentInstance } from 'vue';
import { orderInputTrans } from './config';

const app = getCurrentInstance();

const props = withDefaults(
  defineProps<{
    visible: boolean;
    orderTriggerInput: Record<string, KungfuApi.KfTradeValueCommonData>;
  }>(),
  {
    visible: false,
  },
);

defineEmits<{
  (e: 'confirm'): void;
  (e: 'update:visible', visible: boolean): void;
  (e: 'close'): void;
}>();

const { modalVisible, closeModal } = useModalVisible(props.visible);

const formData = computed<Record<string, KungfuApi.KfTradeValueCommonData>>(
  () => {
    const transKeys = Object.keys(orderInputTrans);
    const results = Object.keys(props.orderTriggerInput).reduce((pre, key) => {
      if (props.orderTriggerInput[key].name !== '' && transKeys.includes(key)) {
        pre[key] = props.orderTriggerInput[key];
      }
      return pre;
    }, {});
    return results;
  },
);

function handleConfirm() {
  app && app.emit('confirm');
  closeModal();
}
</script>

<template>
  <a-modal
    v-model:visible="modalVisible"
    class="kf-order-trigger-confirm-modal"
    :width="420"
    :title="$t('tradingConfig.order_trigger_title')"
    :destroy-on-close="true"
    @cancel="closeModal"
    @ok="handleConfirm"
  >
    <div class="order-trigger-content-wrap">
      <p class="color-default" style="margin-bottom: 10px">
        {{ $t('orderTriggerConfig.make_order_modal_tip') }}
      </p>
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
.kf-order-trigger-confirm-modal {
  .ant-modal-body {
    padding: 24px;
  }
  .order-trigger-content-wrap {
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

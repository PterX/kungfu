<script setup lang="ts">
import { ref, getCurrentInstance } from 'vue';
import { useModalVisible } from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/uiUtils';
import { addOperatorConfig } from './config';
import { AddOperatorTypeEnum } from '@kungfu-trader/kungfu-js-api/typings/enums';

const app = getCurrentInstance();
const props = withDefaults(
  defineProps<{
    visible: boolean;
  }>(),
  {
    visible: false,
  },
);

const { modalVisible, closeModal } = useModalVisible(props.visible);
const selectedOpertorType = ref<AddOperatorTypeEnum>(AddOperatorTypeEnum.File);

function handleConfirm() {
  app && app.emit('confirm', selectedOpertorType.value);
  closeModal();
}
</script>
<template>
  <a-modal
    class="add-operator-type-modal"
    :width="320"
    v-model:visible="modalVisible"
    :title="$t('operatorConfig.add_operator_type.title')"
    :destroyOnClose="true"
    @cancel="closeModal"
    @ok="handleConfirm"
  >
    <a-radio-group v-model:value="selectedOpertorType">
      <a-radio
        v-for="item in addOperatorConfig.options"
        :key="item.value"
        :value="item.value"
        :style="{
          height: '36px',
          'line-height': '36px',
          'font-size': '16px',
          'min-width': '45%',
        }"
      >
        <span class="operator-type__txt">{{ item.label }}</span>
      </a-radio>
    </a-radio-group>
  </a-modal>
</template>
<style lang="less">
.add-operator-type-modal {
  .operator-type__txt {
    margin-right: 8px;
    font-weight: bold;
  }
}
</style>

<script setup lang="ts">
import { ref, getCurrentInstance } from 'vue';
import { useModalVisible } from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/uiUtils';
import KfConfigSettingsForm from '@kungfu-trader/kungfu-app/src/renderer/components/public/KfConfigSettingsForm.vue';
import { initFormStateByConfig } from '@kungfu-trader/kungfu-js-api/utils/busiUtils';
import { addOperatorConfig } from './config';

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
const formState = ref(initFormStateByConfig(addOperatorConfig, {}));

function handleConfirm() {
  app && app.emit('confirm', formState.value.type);
  closeModal();
}
</script>
<template>
  <a-modal
    class="add-operator-modal"
    :width="320"
    v-model:visible="modalVisible"
    :title="$t('operatorConfig.add_operator_type.title')"
    :destroyOnClose="true"
    @cancel="closeModal"
    @ok="handleConfirm"
  >
    <KfConfigSettingsForm
      ref="formRef"
      v-model:formState="formState"
      :config-settings="addOperatorConfig"
      change-type="add"
      :label-col="6"
      :wrapper-col="14"
    ></KfConfigSettingsForm>
  </a-modal>
</template>

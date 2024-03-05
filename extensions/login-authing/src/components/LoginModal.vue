<script setup lang="ts">
import KfConfigSettingsFormVue from '@kungfu-trader/kungfu-app/src/renderer/components/public/KfConfigSettingsForm.vue';
import { useModalVisible } from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/uiUtils';
import { LoginTypeEnum, loginConfigs, LoginTypes } from '../configs';
import { useLogin } from '../utils';
import VueI18n from '@kungfu-trader/kungfu-js-api/language';

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
  (e: 'loginSuccess'): void;
  (e: 'loginFailed'): void;
  (e: 'update:visible', visible: boolean): void;
  (e: 'close'): void;
}>();
const { modalVisible, closeModal } = useModalVisible(props.visible);
const {
  formRef,
  currentLoginType,
  formState,
  sentCodeTime,
  codeAddonAfterText,
  handleSendCode,
  loginLoading,
  handleConfirm,
} = useLogin();

const phoneNumberValidator = (rule, value) => {
  if (!value) {
    return Promise.reject(new Error(t('validate.mandatory')));
  }

  if (!/^[1][1-9][0-9]{9}$/.test(value)) {
    return Promise.reject(new Error(t('loginAuthing.phoneInvalid')));
  }

  return Promise.resolve();
};
</script>
<template>
  <a-modal
    :width="520"
    class="kf-authing-login-modal"
    :visible="modalVisible"
    :title="$t('loginAuthing.login')"
    :destroy-on-close="true"
    @cancel="closeModal"
  >
    <a-tabs
      v-model:activeKey="currentLoginType"
      class="kf-authing-login-modal-tabs"
      size="small"
    >
      <a-tab-pane
        v-for="[loginType, config] in Object.entries(LoginTypes)"
        :key="+loginType"
        :tab="config.name"
      ></a-tab-pane>
    </a-tabs>

    <a-form
      v-if="currentLoginType === LoginTypeEnum.PhoneWithCode"
      ref="formRef"
      class="kf-authing-config-form"
      :model="formState"
      :label-col="{ span: 6 }"
      :label-wrap="true"
      :wrapper-col="{ span: 14 }"
      label-align="right"
      :colon="false"
      :scroll-to-first-error="true"
      layout="horizontal"
    >
      <a-form-item
        :label="$t('loginAuthing.phone')"
        name="phone"
        :rules="[
          {
            type: 'number',
            required: true,
            trigger: ['change', 'blur'],
            validator: phoneNumberValidator,
          },
        ]"
      >
        <a-input-number
          v-model:value="formState.phone"
          :controls="false"
        ></a-input-number>
      </a-form-item>
      <a-form-item
        :label="$t('loginAuthing.code')"
        name="code"
        :rules="[
          {
            type: 'number',
            required: true,
            message: $t('validate.mandatory'),
            trigger: ['change', 'blur'],
          },
        ]"
      >
        <a-input-number v-model:value="formState.code" :controls="false">
          <template #addonAfter>
            <a-button
              type="text"
              size="small"
              :disabled="sentCodeTime > 0"
              @click="handleSendCode"
            >
              {{ codeAddonAfterText }}
            </a-button>
          </template>
        </a-input-number>
      </a-form-item>
    </a-form>
    <KfConfigSettingsFormVue
      v-else
      ref="formRef"
      v-model:formState="formState"
      :config-settings="loginConfigs[currentLoginType]"
    ></KfConfigSettingsFormVue>

    <template #footer>
      <a-button
        key="submit"
        type="primary"
        :loading="loginLoading"
        @click="() => handleConfirm().then(() => closeModal())"
      >
        {{ $t('loginAuthing.confirm') }}
      </a-button>
    </template>
  </a-modal>
</template>
<style lang="less">
.kf-authing-login-modal-tabs {
  .ant-tabs-nav {
    width: 72%;
    margin: 0 auto 24px;
  }
}

.kf-authing-config-form {
  .ant-input-number {
    width: 80%;
  }

  .ant-input-number-group-wrapper {
    width: 80%;
    .ant-input-number {
      width: 100%;
    }
  }
}
</style>

<template>
  <div v-if="isLogin" @click="handleOpenWallet">
    <WalletOutlined />
    <span style="margin-left: 4px">
      {{
        `${t('awsWallet.balance')}: ${dealKfNumber(
          walletStore.currentBalance,
          2,
        )}${t('awsWallet.unit')}`
      }}
    </span>
  </div>

  <wallet-detail v-model:toggle="toggle"></wallet-detail>
</template>

<script lang="ts" setup>
import {
  ref,
  computed,
  onMounted,
  getCurrentInstance,
  onBeforeUnmount,
} from 'vue';
import { shell } from '@electron/remote';

import { WalletOutlined } from '@ant-design/icons-vue';
import walletDetail from './walletDetail.vue';

import VueI18n from '@kungfu-trader/kungfu-js-api/language';
import { dealKfNumber } from '@kungfu-trader/kungfu-js-api/utils/commonUtils';
import { useAuthingCredential } from '@kungfu-trader/kfx-ui-login-authing/src/utils/externalUtils';

import { useWalletStore } from '../../store';
import { AwsWalletKeys, RechargeUrl } from '../../configs';

const { t } = VueI18n.global;
const app = getCurrentInstance();
const walletStore = useWalletStore();
const { getCurrentCredential } = useAuthingCredential();
const toggle = ref(false);

const isLogin = computed(() => {
  return !!getCurrentCredential();
});

onMounted(() => {
  walletStore.startPolling();

  const sub = app?.proxy?.$globalBus.subscribe((data) => {
    if (data.tag === AwsWalletKeys.ShowRecords) {
      toggle.value = true;
    }
    if (data.tag === AwsWalletKeys.ToRecharge) {
      shell.openExternal(RechargeUrl);
    }

    if (data.tag === AwsWalletKeys.RefreshData) {
      if (!walletStore.currentWallet) return;

      walletStore.refreshAll();
    }
  });

  onBeforeUnmount(() => {
    sub?.unsubscribe();
  });
});

window.addEventListener(
  'focus',
  function () {
    walletStore.startPolling();
  },
  false,
);

window.addEventListener(
  'blur',
  function () {
    walletStore.stopPolling();
  },
  false,
);

const handleOpenWallet = () => {
  toggle.value = !toggle.value;
};
</script>

<style lang="less"></style>

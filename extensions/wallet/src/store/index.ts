import { ref, watch, computed } from 'vue';
import { defineStore } from 'pinia';
import { Wallet } from '../utils/Wallet';
import { useAuthingCredential } from '@kungfu-trader/kfx-ui-login-authing/src/utils/externalUtils';
import { Credential } from '@kungfu-trader/kfx-ui-login-authing/src/typings';

export const useWalletStore = defineStore('wallet', () => {
  const { getCurrentCredential } = useAuthingCredential();
  const currentWallet = ref<Wallet | null>(null);
  const dataSyncing = ref(false);

  const currentBalance = computed(() => {
    if (currentWallet.value && currentWallet.value.walletData) {
      const originBalance = currentWallet.value.walletData.balance || 0;

      return originBalance;
    }

    return null;
  });

  const credential = computed(() => getCurrentCredential());

  const setWallet = async (credential: Credential, force = false) => {
    if (force && currentWallet.value) return;
    if (!credential.access_token) return;

    const wallet = new Wallet(credential.access_token);
    dataSyncing.value = true;
    await wallet
      .initWallet()
      .then(() => {
        currentWallet.value = wallet;
        currentWallet.value.loadTransactions();
      })
      .finally(() => {
        dataSyncing.value = false;
      });
  };

  watch(
    () => credential.value,
    (newVal) => {
      if (newVal) {
        if (currentWallet.value) return;
        setWallet(newVal, true);
      } else {
        currentWallet.value = null;
      }
    },
    {
      immediate: true,
    },
  );

  const refreshAll = () => {
    if (!currentWallet.value || !credential.value) return Promise.resolve();

    dataSyncing.value = true;
    return currentWallet.value
      .refreshAll()
      .then(() => {
        dataSyncing.value = false;
      })
      .catch((err) => {
        console.error(err);
        return Promise.reject(err);
      })
      .finally(() => {
        dataSyncing.value = false;
      });
  };

  const startPolling = () => {
    if (globalThis.walletPollingTimer) return;
    globalThis.walletPollingTimer = setInterval(() => {
      refreshAll().catch((err) => {
        console.error(err);
      });
    }, 10000);
  };

  const stopPolling = () => {
    globalThis.walletPollingTimer &&
      clearInterval(globalThis.walletPollingTimer);
    globalThis.walletPollingTimer = null;
  };

  return {
    currentWallet,
    currentBalance,
    refreshAll,
    dataSyncing,
    startPolling,
    stopPolling,
  };
});

<script setup lang="ts">
import {
  getCurrentInstance,
  onBeforeUnmount,
  onMounted,
  ref,
  computed,
} from 'vue';
import { shell } from 'electron';
import { storeToRefs } from 'pinia';
import { LoginOutlined, UserOutlined } from '@ant-design/icons-vue';

import VueI18n from '@kungfu-trader/kungfu-js-api/language';
import { messagePrompt } from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/uiUtils';
import { debounce } from '@kungfu-trader/kungfu-js-api/utils/commonUtils';

import LoginModalVue from '../../components/LoginModal.vue';
import { CURRENT_STAGE, ConnectUsUrl, LoginAuthingKeys } from '../../configs';
import { useAuthLoginStore } from '../../store';
import { clearCredentials, readCredentials } from '../../utils/credential';
import { useAuthingCredential } from '../../utils/externalUtils';
import { kfLoginAuthing } from '../../io';
const { t } = VueI18n.global;

const app = getCurrentInstance();
const authLoginStore = useAuthLoginStore();
const loginModalShow = ref<boolean>(false);
const isLoggedIn = ref<boolean>(false);
const { currentAccount, credential } = storeToRefs(authLoginStore);
const { checkCredential, CredentialStatus, triggerCredentialStatus } =
  useAuthingCredential();

// 每次打开都检查登录状态
readCredentials(CURRENT_STAGE).then((res) => {
  if (res?.access_token && res?.id_token && res?.gen_time) {
    const status = checkCredential(res);
    if (status === CredentialStatus.Valid) {
      authLoginStore.setCredentials(res);

      kfLoginAuthing.getCurrentUserInfo(res).then((user) => {
        authLoginStore.setCurrentAccount(user);
        isLoggedIn.value = true;
      });
    } else {
      triggerCredentialStatus(status);
      loginModalShow.value = true;
    }
  }
});

const currentAccountName = computed(() =>
  currentAccount.value
    ? `${t('loginAuthing.username')}: ${
        currentAccount.value.nickname ||
        currentAccount.value.name ||
        currentAccount.value.phone
      }`
    : '',
);

onMounted(() => {
  if (app?.proxy) {
    const expiredCallback = debounce(() => {
      messagePrompt().warn(t('loginAuthing.expired'));
      logout(false).then(() => (loginModalShow.value = true));
    });
    const notLoginCallback = debounce(() => {
      messagePrompt().warn(t('loginAuthing.notLogin'));
      loginModalShow.value = true;
    });

    const sub = app.proxy.$globalBus.subscribe((data) => {
      if (data.tag === LoginAuthingKeys.CredentialExpired) {
        expiredCallback();
      }
      if (data.tag === LoginAuthingKeys.NotLogin) {
        notLoginCallback();
      }
      if (data.tag === LoginAuthingKeys.CallLogin) {
        loginModalShow.value = true;
      }
      if (data.tag === LoginAuthingKeys.CallLogout) {
        logout(false);
      }
      if (data.tag === LoginAuthingKeys.ConcatUs) {
        shell.openExternal(ConnectUsUrl);
      }
    });

    onBeforeUnmount(() => {
      sub.unsubscribe();
    });
  }
});

function logout(byManual = true) {
  return kfLoginAuthing
    .logoutByAccessToken(credential.value?.access_token || '')
    .finally(() => {
      return clearCredentials(CURRENT_STAGE)
        .then(() => {
          authLoginStore.setCurrentAccount(null);
          authLoginStore.setCredentials(null);
          isLoggedIn.value = false;
        })
        .then(() => {
          byManual && messagePrompt().success();
        });
    })
    .finally(() => {
      app?.proxy?.$globalBus.next({
        tag: LoginAuthingKeys.LoggedOut,
      });
    });
}

function handleLoginSuccess() {
  loginModalShow.value = false;
  isLoggedIn.value = true;
}

function handleLoginFailed() {
  isLoggedIn.value = false;
}

function handleOpenWallet() {
  app?.proxy?.$globalBus.next({
    tag: 'aws-wallet:show-records',
  });
}

function handleLogout() {
  return logout();
}
</script>

<template>
  <a-popover
    :trigger="[isLoggedIn ? 'hover' : 'click']"
    placement="rightBottom"
    :align="{ offset: [-10, -16] }"
    overlay-class-name="login-popover"
  >
    <div class="kf-authing-btn__warp">
      <login-outlined v-if="!isLoggedIn" />
      <user-outlined v-else />
    </div>
    <template #title>
      <div v-if="isLoggedIn">
        {{ currentAccountName }}
      </div>
    </template>
    <template #content>
      <template v-if="!isLoggedIn">
        <div class="login-menu-item" @click="loginModalShow = true">
          <span>
            {{ $t('loginAuthing.login') }}
          </span>
        </div>
      </template>
      <template v-else>
        <div class="login-menu-item" @click="handleOpenWallet">
          <span>
            {{ $t('loginAuthing.transaction') }}
          </span>
        </div>
        <div class="login-menu-item" @click="handleLogout">
          <span>
            {{ $t('loginAuthing.logout') }}
          </span>
        </div>
      </template>
    </template>
  </a-popover>
  <LoginModalVue
    v-if="loginModalShow"
    v-model:visible="loginModalShow"
    @login-success="handleLoginSuccess"
    @login-failed="handleLoginFailed"
  ></LoginModalVue>
</template>

<style lang="less">
.login-popover {
  .ant-popover-inner {
    box-shadow: 0 3px 6px -4px rgb(0 0 0 / 48%),
      -12px 0px 16px 0 rgb(0 0 0 / 32%), 0 9px 28px 8px rgb(0 0 0 / 20%) !important;
  }
}

.login-menu-item {
  white-space: nowrap;
  width: 100%;
  height: 28px;
  display: flex;
  align-items: center;
  cursor: pointer;

  &:hover {
    color: #faad14;
  }
}
</style>

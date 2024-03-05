import { LoginTypeEnum } from './../configs/index';
import { ref, getCurrentInstance, computed } from 'vue';

import VueI18n from '@kungfu-trader/kungfu-js-api/language';
import { initFormStateByConfig } from '@kungfu-trader/kungfu-js-api/utils/busiUtils';
import { messagePrompt } from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/uiUtils';
import {
  CURRENT_STAGE,
  LocalStorageKeys,
  LoginAuthingKeys,
  loginConfigs,
} from '../configs';
import { kfLoginAuthing } from '../io';
import { writeCredentials } from './credential';
import { useAuthLoginStore } from '../store';

const { t } = VueI18n.global;
const message = messagePrompt();

export const setLoginFormIntoLocalStorage = (
  loginType: LoginTypeEnum,
  formState: Record<string, KungfuApi.KfConfigValue>,
) => {
  const loginForm = JSON.parse(
    localStorage.getItem(LocalStorageKeys.LoginForm) || '{}',
  ) as Record<LoginTypeEnum, Record<string, KungfuApi.KfConfigValue>>;

  if (loginType === LoginTypeEnum.PhoneWithCode && 'code' in formState)
    delete formState.code;

  loginForm[loginType] = formState;
  localStorage.setItem(LocalStorageKeys.LoginForm, JSON.stringify(loginForm));
};

export const getLoginFormFromLocalStorage = (loginType: LoginTypeEnum) => {
  const initLoginForm = JSON.parse(
    localStorage.getItem(LocalStorageKeys.LoginForm) ||
      JSON.stringify({ [loginType]: {} }),
  ) as Record<LoginTypeEnum, Record<string, KungfuApi.KfConfigValue>>;

  if (
    loginType === LoginTypeEnum.PhoneWithCode &&
    'code' in initLoginForm[loginType]
  )
    delete initLoginForm[loginType].code;

  const formState = initFormStateByConfig(
    loginConfigs[loginType],
    initLoginForm[loginType],
  );

  return formState;
};

export const useLogin = () => {
  const app = getCurrentInstance();
  const DefaultLoginType = LoginTypeEnum.PhoneWithCode;
  const currentLoginType = ref(DefaultLoginType);
  const formState = ref<Record<string, KungfuApi.KfConfigValue>>(
    getLoginFormFromLocalStorage(DefaultLoginType),
  );
  const formRef = ref();
  const loginLoading = ref<boolean>(false);
  const sentCodeTime = ref(0);
  const codeAddonAfterText = computed(() => {
    if (sentCodeTime.value > 0) {
      return `${sentCodeTime.value} s`;
    }
    return t('loginAuthing.getCode');
  });

  const handleSendCode = () => {
    if (currentLoginType.value === LoginTypeEnum.PhoneWithCode) {
      return formRef.value.validate(['phone']).then(() => {
        kfLoginAuthing
          .sendCodeMessage(`${formState.value.phone}`)
          .then(() => {
            sentCodeTime.value = 60;
            const timer = setInterval(() => {
              sentCodeTime.value -= 1;
              if (sentCodeTime.value === 0) {
                clearInterval(timer);
              }
            }, 1000);
          })
          .catch((err) => {
            console.error(err);
            message.error(err.message || t('loginAuthing.operationFailed'));
          });
      });
    }

    return Promise.resolve();
  };

  const login = (formState: Record<string, KungfuApi.KfConfigValue>) => {
    switch (currentLoginType.value) {
      case LoginTypeEnum.PhoneWithCode:
        return kfLoginAuthing.loginByPhonePassCode(
          `${formState.phone}`,
          `${formState.code}`,
        );
      case LoginTypeEnum.Phone:
        return kfLoginAuthing.loginByPhoneAndPassword(
          formState.phone,
          formState.password,
        );
      case LoginTypeEnum.Username:
        return kfLoginAuthing.loginByUsernameAndPassword(
          formState.username,
          formState.password,
        );
      case LoginTypeEnum.Email:
        return kfLoginAuthing.loginByEmailAndPassword(
          formState.email,
          formState.password,
        );
    }
  };

  const handleConfirm = () => {
    loginLoading.value = true;
    return new Promise<void>((resolve) => {
      formRef.value
        .validate()
        .then(() =>
          login(formState.value).then((data) => {
            return writeCredentials(CURRENT_STAGE, data).then(() => {
              useAuthLoginStore().setCredentials(data);
              setLoginFormIntoLocalStorage(
                currentLoginType.value,
                formState.value,
              );
              app && app.emit('loginSuccess');
              return kfLoginAuthing.getCurrentUserInfo(data).then((user) => {
                useAuthLoginStore().setCurrentAccount(user);
                message.success(t('loginAuthing.succeed'));
                app?.proxy?.$globalBus.next({
                  tag: LoginAuthingKeys.LoggedIn,
                  credential: data,
                  user,
                });
                resolve();
              });
            });
          }),
        )
        .catch((err: Error) => {
          if (err.message) {
            const networkErrors = ['timeout', 'Network Error', 'networkError'];
            if (networkErrors.some((error) => err.message.includes(error))) {
              err.message = t('loginAuthing.networkError');
            }
            message.error(`${t('loginAuthing.failed')}: ${err.message}`);
            app && app.emit('loginFailed');
          }
          console.error(err);
        })
        .finally(() => {
          loginLoading.value = false;
        });
    });
  };

  return {
    formRef,
    currentLoginType,
    formState,
    sentCodeTime,
    codeAddonAfterText,
    handleSendCode,
    loginLoading,
    handleConfirm,
  };
};

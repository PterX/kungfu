import VueI18n from '@kungfu-trader/kungfu-js-api/language';
import { CURRENT_STAGE } from './authing';
export * from './authing';

const { t } = VueI18n.global;

const urlPrefix = CURRENT_STAGE === 'prod' ? 'www' : CURRENT_STAGE;

export const ConnectUsUrl = `https://${urlPrefix}.kungfu-trader.com/index.php/consult/`;

export const LocalStorageKeys = {
  LoginForm: 'AuthingLoginFrom',
  Credentials: 'AuthingCredentials',
} as const;

export const LoginAuthingKeys = {
  NotLogin: 'authing:not-login',
  CredentialExpired: 'authing:credential-expired',
  CallLogin: 'authing:call-login',
  CallLogout: 'authing:call-logout',
  CallSyncData: 'authing:call-sync-data',
  LoggedIn: 'authing:logged-in',
  LoggedOut: 'authing:logged-out',
  DataSynced: 'authing:data-synced',
  ConcatUs: 'authing:concat-us',
} as const;

export enum LoginTypeEnum {
  PhoneWithCode,
  Phone,
  Username,
  Email,
}

export const loginConfigs: Record<LoginTypeEnum, KungfuApi.KfConfigItem[]> = {
  [LoginTypeEnum.PhoneWithCode]: [
    {
      key: 'phone',
      name: t('loginAuthing.phone'),
      type: 'str',
      required: true,
      tip: t('loginAuthing.pleaseEnter', { value: t('loginAuthing.phone') }),
    },
    {
      key: 'code',
      name: t('loginAuthing.code'),
      type: 'str',
      required: true,
      tip: t('loginAuthing.pleaseEnter', { value: t('loginAuthing.code') }),
    },
  ],
  [LoginTypeEnum.Phone]: [
    {
      key: 'phone',
      name: t('loginAuthing.phone'),
      type: 'str',
      required: true,
      tip: t('loginAuthing.pleaseEnter', { value: t('loginAuthing.phone') }),
    },
    {
      key: 'password',
      name: t('loginAuthing.password'),
      type: 'password',
      required: true,
      tip: t('loginAuthing.pleaseEnter', { value: t('loginAuthing.password') }),
    },
  ],
  [LoginTypeEnum.Username]: [
    {
      key: 'username',
      name: t('loginAuthing.username'),
      type: 'str',
      required: true,
      tip: t('loginAuthing.pleaseEnter', { value: t('loginAuthing.username') }),
    },
    {
      key: 'password',
      name: t('loginAuthing.password'),
      type: 'password',
      required: true,
      tip: t('loginAuthing.pleaseEnter', { value: t('loginAuthing.password') }),
    },
  ],
  [LoginTypeEnum.Email]: [
    {
      key: 'email',
      name: t('loginAuthing.email'),
      type: 'str',
      required: true,
      tip: t('loginAuthing.pleaseEnter', { value: t('loginAuthing.email') }),
    },
    {
      key: 'password',
      name: t('loginAuthing.password'),
      type: 'password',
      required: true,
      tip: t('loginAuthing.pleaseEnter', { value: t('loginAuthing.password') }),
    },
  ],
};

export const LoginTypes: Partial<
  Record<LoginTypeEnum, KungfuApi.KfTradeValueCommonData>
> = {
  [LoginTypeEnum.PhoneWithCode]: {
    name: t('loginAuthing.phoneWithCode'),
  },
  // [LoginTypeEnum.Phone]: {
  //   name: t('loginAuthing.phone'),
  // },
  // [LoginTypeEnum.Username]: {
  //   name: t('loginAuthing.username'),
  // },
  // [LoginTypeEnum.Email]: {
  //   name: t('loginAuthing.email'),
  // },
};

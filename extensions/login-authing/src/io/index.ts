import { AuthenticationClient, Models } from 'authing-node-sdk';
import {
  SignInOptionsDto,
  LoginTokenRespDto,
  LoginTokenResponseDataDto,
} from 'authing-node-sdk/dist/models';
import { AuthingAppConfigMap, CURRENT_STAGE } from '../configs';
import { Credential, AuthingAppConfig, UserInfo } from '../typings';
import VueI18n from '@kungfu-trader/kungfu-js-api/language';
const { t } = VueI18n.global;

export class KungfuLoginAuthing {
  ac: AuthenticationClient;
  baseLoginOptions: SignInOptionsDto;
  userInfo: UserInfo | null;

  constructor(config: AuthingAppConfig) {
    this.ac = new AuthenticationClient(config);
    this.baseLoginOptions = {
      scope: 'phone profile email openid offline_access backtest',
    };
    this.userInfo = null;
  }

  private postRequest<
    T extends { statusCode: number; data?: object; message: string },
  >({ statusCode, data, message }: T) {
    if (statusCode === 200) {
      return Promise.resolve(data as T['data']);
    }

    const networkErrors = ['timeout', 'Network Error', 'networkError'];
    if (networkErrors.some((error) => message.includes(error))) {
      message = t('loginAuthing.networkError');
    }

    return Promise.reject(new Error(message));
  }

  private preLogin() {
    return Promise.resolve(this.ac.setAccessToken(''));
  }

  private postLogin(res: LoginTokenRespDto) {
    return this.postRequest(res).then((data) => {
      data.access_token && this.ac.setAccessToken(data.access_token);
      return {
        ...data,
        gen_time: Date.now() / 1000,
      } as LoginTokenResponseDataDto & {
        expires_in: number;
        gen_time: number;
      };
    });
  }

  sendCodeMessage(phoneNumber: string) {
    return this.ac
      .sendSms({
        phoneNumber,
        channel: Models.SendSMSDto.channel.CHANNEL_LOGIN,
        phoneCountryCode: '+86',
      })
      .then((res) => this.postRequest(res));
  }

  loginByPhonePassCode(phone: string, passCode: string) {
    return this.preLogin().then(() =>
      this.ac
        .signInByPhonePassCode({
          phone,
          passCode,
          options: { ...this.baseLoginOptions },
        })
        .then((res) => this.postLogin(res)),
    );
  }

  loginByPhoneAndPassword(phone: string, password: string) {
    return this.preLogin().then(() =>
      this.ac
        .signInByPhonePassword({
          phone,
          password,
          options: { ...this.baseLoginOptions },
        })
        .then((res) => this.postLogin(res)),
    );
  }

  loginByUsernameAndPassword(username: string, password: string) {
    return this.preLogin().then(() =>
      this.ac
        .signInByUsernamePassword({
          username,
          password,
          options: { ...this.baseLoginOptions },
        })
        .then((res) => this.postLogin(res)),
    );
  }

  loginByEmailAndPassword(email: string, password: string) {
    return this.preLogin().then(() =>
      this.ac
        .signInByEmailPassword({
          email,
          password,
          options: { ...this.baseLoginOptions },
        })
        .then((res) => this.postLogin(res)),
    );
  }

  logoutByAccessToken(token: string) {
    if (!token) return Promise.resolve();
    this.ac.setAccessToken('');
    return this.ac.revokeToken(token);
  }

  getNewAccessTokenByRefreshToken(credential: Credential) {
    if (!credential.access_token)
      return Promise.reject('Access token not found');
    if (!credential.refresh_token)
      return Promise.reject('Refresh token not found');

    this.ac.setAccessToken('');

    return this.ac
      .getNewAccessTokenByRefreshToken(credential.refresh_token)
      .then((data: Required<Credential>) => {
        this.ac.setAccessToken(data.access_token);
        data.gen_time = Date.now() / 1000;
        return data;
      })
      .catch((err) => {
        this.ac.setAccessToken(credential.access_token as string);
        return Promise.reject(err);
      });
  }

  getCurrentUserInfo(credential: Credential) {
    const { id_token, access_token } = credential;
    if (id_token && access_token) {
      this.ac.setAccessToken(access_token);
      return Promise.all([
        this.ac.getUserInfoByAccessToken(access_token),
        this.ac
          .getProfile({
            withCustomData: true,
            withDepartmentIds: true,
            withIdentities: true,
          })
          .then((res) => this.postRequest(res)),
      ])
        .then((data) => {
          this.userInfo = { ...data[0], ...data[1] } as unknown as UserInfo;
          return this.userInfo;
        })
        .catch((err) => {
          this.ac.setAccessToken('');
          console.error(err);
          return Promise.reject(err);
        });
    }

    return Promise.reject('Access token not found');
  }

  getUserWithCustomData(credential: Credential) {
    const { id_token, access_token } = credential;
    if (id_token && access_token) {
      this.ac.setAccessToken(access_token);
      return this.ac
        .getProfile({
          withCustomData: true,
          withDepartmentIds: true,
          withIdentities: true,
        })
        .then((res) => this.postRequest(res));
    }

    return Promise.reject('Access token not found');
  }
}

const kfLoginAuthing = new KungfuLoginAuthing(
  AuthingAppConfigMap[CURRENT_STAGE],
);

globalThis.kfLoginAuthing = kfLoginAuthing;

export { kfLoginAuthing };

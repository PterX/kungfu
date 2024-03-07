import { defineStore } from 'pinia';
import { Credential, UserInfo } from '../typings';

export interface AuthLoginStoreState {
  currentAccount: UserInfo | null;
  credential: Credential | null;
}

export const useAuthLoginStore = defineStore('authLogin', {
  state: () =>
    ({
      currentAccount: null,
      credential: {},
    } as AuthLoginStoreState),
  actions: {
    setCurrentAccount(account: UserInfo | null) {
      this.currentAccount = account;
    },
    setCredentials(credential: Credential | null) {
      this.credential = credential;
    },
  },
});

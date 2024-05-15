import { getCurrentInstance } from 'vue';
import { storeToRefs } from 'pinia';
import { UserDto } from 'authing-node-sdk/dist/models';

import { Credential } from '../typings';
import { useAuthLoginStore } from '../store';
import { LoginAuthingKeys } from '../configs';
import { kfLoginAuthing } from '../io';

export const useAuthingCredential = () => {
  const app = getCurrentInstance();
  const authLoginStore = useAuthLoginStore();
  const { currentAccount, credential } = storeToRefs(authLoginStore);

  enum CredentialStatus {
    Valid = 'valid',
    Expired = 'expired',
    Invalid = 'invalid',
  }
  const checkCredential = (credential: Credential | null): CredentialStatus => {
    if (
      credential &&
      credential.access_token &&
      credential.id_token &&
      credential.gen_time
    ) {
      const now = Date.now() / 1000;
      const genTime = credential.gen_time;
      const expiresIn = credential.expires_in - 10;
      if (now - genTime < expiresIn) return CredentialStatus.Valid;

      return CredentialStatus.Expired;
    }

    return CredentialStatus.Invalid;
  };

  const triggerCredentialStatus = (status: CredentialStatus) => {
    const busEventTag =
      status === CredentialStatus.Expired
        ? LoginAuthingKeys.CredentialExpired
        : LoginAuthingKeys.NotLogin;
    app?.proxy?.$globalBus.next({
      tag: busEventTag,
    });
  };

  const getCurrentCredential = (silent = true): Credential | false => {
    const status = checkCredential(credential.value);

    if (status !== CredentialStatus.Valid) {
      authLoginStore.setCredentials(null);
      authLoginStore.setCurrentAccount(null);

      if (!silent) {
        triggerCredentialStatus(status);
      }

      return false;
    }

    return credential.value as Credential;
  };

  const checkAccountAccess = (user: UserDto, targetAccesses: string[]) => {
    return targetAccesses.every((access) => user.customData[access]);
  };

  const checkCurrentAccountAccess = (targetAccesses: string[]) => {
    if (!currentAccount.value) return false;
    return checkAccountAccess(currentAccount.value, targetAccesses);
  };

  const checkCredentialAccess = (
    targetAccesses: string[],
    updateAccess = true,
  ): Promise<false | Credential> => {
    const credential = getCurrentCredential();
    if (!credential) return Promise.resolve(false);
    return kfLoginAuthing
      .getUserWithCustomData(credential)
      .then((user) => {
        if (!checkAccountAccess(user, targetAccesses)) {
          if (updateAccess) {
            const authingLoginStore = useAuthLoginStore();
            const { currentAccount } = authingLoginStore;
            authingLoginStore.setCurrentAccount({ ...currentAccount, ...user });
          }

          return false;
        }

        return credential as Credential;
      })
      .catch((err) => {
        console.error(err);
        return false;
      });
  };

  return {
    currentAccount,
    CredentialStatus,
    checkCredential,
    triggerCredentialStatus,
    checkAccountAccess,
    getCurrentCredential,
    checkCredentialAccess,
    checkCurrentAccountAccess,
  };
};

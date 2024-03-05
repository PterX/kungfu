import path from 'path';
import fse from 'fs-extra';
import { Stage, Credential, Credentials } from '../typings';
import { KF_CONFIG_ROOT_DIR } from '@kungfu-trader/kungfu-js-api/config/pathConfig';
import { addFileSync } from '@kungfu-trader/kungfu-js-api/utils/fileUtils';

export const KF_CREDENTIALS_PATH = path.join(
  KF_CONFIG_ROOT_DIR,
  'authingCredentials.json',
);
if (!fse.pathExistsSync(KF_CREDENTIALS_PATH)) {
  addFileSync('', KF_CREDENTIALS_PATH, 'file', '{}');
}

export function readCredentials(stage: Stage): Promise<Credential | null> {
  try {
    const credentials = fse.readJSONSync(KF_CREDENTIALS_PATH) as Credentials;
    return Promise.resolve(credentials[stage] ?? null);
  } catch (err) {
    console.error(`Failed to read credentials: ${err}`);
    return Promise.reject();
  }
}

export function writeCredentials(
  stage: Stage,
  credential: Partial<Credential>,
): Promise<void> {
  try {
    const credentials = fse.readJSONSync(
      KF_CREDENTIALS_PATH,
    ) as Partial<Credential>;
    credentials[stage] = credential;
    fse.writeJSONSync(KF_CREDENTIALS_PATH, credentials);
    return Promise.resolve();
  } catch (err) {
    console.error(`Failed to write credentials: ${err}`);
    return Promise.reject();
  }
}

export function clearCredentials(stage: Stage) {
  return writeCredentials(stage, {});
}

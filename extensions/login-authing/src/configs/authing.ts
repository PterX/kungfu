import os from 'os';
import path from 'path';
import { AuthingAppConfig, Stage } from '../typings';
import { KF_CONFIG_DIR } from '@kungfu-trader/kungfu-js-api/config/pathConfig';
import fse from 'fs-extra';

const stageConfigPath = path.join(KF_CONFIG_DIR, 'stage.json');
const stageConfig: { currentStage?: Stage } = fse.existsSync(stageConfigPath)
  ? fse.readJsonSync(stageConfigPath) || {}
  : {};

export const DEFAULT_CURRENT_STAGE =
  process.env.NODE_ENV === 'production' ? 'prod' : 'dev';

export const CURRENT_STAGE = stageConfig.currentStage || DEFAULT_CURRENT_STAGE;

console.log(`current stage: ${CURRENT_STAGE}`);

// define path to credentials file
export const SLS_KUNGFU_CREDENTIALS_PATH = `${os.homedir()}/.serverless-kungfu/credentials.yaml`;
export const SLS_KUNGFU_CREDENTIALS_DIRECTORY = path.dirname(
  SLS_KUNGFU_CREDENTIALS_PATH,
);

export const AuthingAppConfigMap: Record<Stage, AuthingAppConfig> = {
  dev: {
    appId: '656839e2d2d74b312123455c',
    appSecret: 'eed96a7726e3e40ce1648f514f05fcc1',
    appHost: 'https://serverless-kungfu-dev.authing.cn',
  },
  alpha: {
    appId: '656ff475456425a56da6c42c',
    appSecret: 'e2e791fa27594b67eb0b48b15881af7b',
    appHost: 'https://serverless-kungfu-alpha.authing.cn',
  },
  prod: {
    appId: '643cfccef8215efb67ebefa5',
    appSecret: '8730916236a807d1f1406a2d5baa5b3c',
    appHost: 'https://serverless-kungfu.authing.cn',
  },
};

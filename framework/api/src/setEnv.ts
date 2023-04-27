import { readRootPackageJsonSync } from '@kungfu-trader/kungfu-js-api/utils/fileUtils';

const packageJson = readRootPackageJsonSync();
const externalEnv = packageJson.kungfuCraft?.env;
if (externalEnv && typeof externalEnv === 'object') {
  Object.keys(externalEnv).forEach((key) => {
    const curEnvValue = externalEnv[key];
    if (typeof curEnvValue === 'string') process.env[key] = curEnvValue;
  });
}

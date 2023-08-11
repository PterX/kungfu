import os from 'os';
import path from 'path';
import fse from 'fs-extra';

if (process.env.APP_TYPE === 'main' || process.env.APP_TYPE === 'renderer') {
  // globalThis.__kfResourcesPath 是一个容易出错的问题, 需要每个调用pathconfig的进程都需要注册这个值
  globalThis.__kfResourcesPath = process.resourcesPath;
} else {
  // cli + uiExtension are in the same way
  globalThis.__kfResourcesPath = path
    .resolve(__dirname, '..', '..', '..')
    .replace(/\\/g, '\\\\');
}

if (process.env.NODE_ENV === 'development') {
  globalThis.__publicResources = `${__resources}`;
} else {
  globalThis.__publicResources =
    process.env.APP_PUBLIC_DIR ||
    path.join(globalThis.__kfResourcesPath, 'app', 'dist', 'public');
}

globalThis.__runtimeDir = globalThis.__runtimeDir || path.resolve(__dirname);

export const getDefaultHomePath = (): string => {
  switch (os.platform()) {
    case 'darwin':
      return path.join(
        os.homedir(),
        'Library',
        'Application Support',
        'kungfu',
      );
    case 'win32':
      return path.join(os.homedir(), 'AppData', 'Roaming', 'kungfu');
    case 'linux':
      return path.join(os.homedir(), '.config', 'kungfu');
    default:
      throw new Error(`Unsupported platform ${os.platform()}`);
  }
};

export const KF_APP_RUNTIME_DIR =
  process.env.KF_APP_RUNTIME_DIR ||
  path.join(globalThis.__kfResourcesPath, 'app');

const getHomePath = (): string => {
  const defaultHomePath = getDefaultHomePath();
  const kfConfigJsonPath = path.join(
    KF_APP_RUNTIME_DIR,
    'config',
    'kfConfig.json',
  );
  if (fse.existsSync(kfConfigJsonPath)) {
    const kfConfigJson = fse.readJSONSync(kfConfigJsonPath) as Record<
      string,
      Record<string, KungfuApi.KfConfigValue>
    >;
    if (kfConfigJson.system.homePath) {
      return kfConfigJson.system.homePath;
    }
  }

  return defaultHomePath;
};

export const KF_HOME_BASE_DIR_RESOLVE: string = getHomePath();

if (process.env.APP_TYPE === 'renderer') {
  console.log('process.env.NODE_ENV', process.env.NODE_ENV);
}

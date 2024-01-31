import semver from 'semver';
import { app, ipcMain, BrowserWindow } from 'electron';
import { autoUpdater } from 'electron-updater';
import { getGlobalStorage } from '@kungfu-trader/kungfu-js-api/utils/globalStorage';
import {
  delayMilliSeconds,
  debounce,
} from '@kungfu-trader/kungfu-js-api/utils/commonUtils';
import { kfLogger } from '@kungfu-trader/kungfu-js-api/utils/logUtils';
import { readRootPackageJsonSync } from '@kungfu-trader/kungfu-js-api/utils/fileUtils';
import {
  downloadProcessUpdate,
  foundNewVersion,
  skipVersion,
  reqRecordBeforeQuit,
  sendUpdatingError,
  startDownloadNewVersion,
  updateNotAvailable,
} from './events';
import { KF_HOME } from '@kungfu-trader/kungfu-js-api/config/pathConfig';
import { killAllBeforeQuit } from './utils';
import { removeTargetFilesInFolder } from '@kungfu-trader/kungfu-js-api/utils/fileUtils';
import axios from 'axios';

import {
  RootConfigJSON,
  Writeable,
  AllPublishOptions,
} from '@kungfu-trader/kungfu-js-api/typings/global';
const globalStorage = getGlobalStorage();
const rootPackageJson = readRootPackageJsonSync();

autoUpdater.logger = kfLogger;
let isRendererReady = false;

const getChannel = (isPrerelease: boolean) => {
  const prefix = 'kungfu-update';
  const baseChannel = isPrerelease ? 'alpha' : 'latest';
  return `${prefix}-${baseChannel}`;
};

const getProjectName = (rootConfigJson: RootConfigJSON): string => {
  if (rootConfigJson.name) {
    const names = rootConfigJson.name.split('/');
    if (names.length) {
      return names[1];
    } else {
      return rootConfigJson.name;
    }
  }

  return `kungfu-project-name-undefined-${new Date().toDateString()}`;
};

const getAlphaReleaseVersion = (version: semver.SemVer) => {
  return `${version.major}.${version.minor}.${version.patch}`;
};

const getNextMinorReleaseVersion = (version: semver.SemVer) => {
  return `${version.major}.${version.minor + 1}.0`;
};

const getLastedSkippedVersion = () => {
  const skippedVersions = globalStorage.getItem('skippedVersions');

  if (skippedVersions) {
    const versionArrays = skippedVersions;
    if (Array.isArray(versionArrays)) {
      return versionArrays[versionArrays.length - 1];
    } else {
      return false;
    }
  } else {
    return false;
  }
};

const getCurrentLastedVersion = (currentVersion: string) => {
  const skipLastedVersion = getLastedSkippedVersion();
  return skipLastedVersion
    ? semver.gt(currentVersion, skipLastedVersion || '')
      ? currentVersion
      : skipLastedVersion
    : currentVersion;
};

function saveSkippedVersion(version: string) {
  const skippedVersions = globalStorage.getItem('skippedVersions');
  if (skippedVersions) {
    const versionArrays = skippedVersions;
    if (Array.isArray(versionArrays)) {
      versionArrays.push(version);
      globalStorage.setItem('skippedVersions', versionArrays);
    } else {
      globalStorage.setItem('skippedVersions', [version]);
    }
  } else {
    globalStorage.setItem('skippedVersions', [version]);
  }
}

function getDefaultTargetVersions(version: semver.SemVer) {
  const isPrerelease = !!version.prerelease.length;
  if (isPrerelease) {
    return [
      semver.inc(version, 'prerelease', 'alpha') || 'kungfu-version-unknow',
      getAlphaReleaseVersion(version),
    ];
  } else {
    return [
      semver.inc(version, 'patch') || 'kungfu-version-unknow',
      getNextMinorReleaseVersion(version),
    ];
  }
}

const download = debounce(() => {
  autoUpdater.downloadUpdate();
}, 1000);

function updateVersion(version: string, incrementType: number): string {
  switch (incrementType) {
    case 1:
      return (
        semver.inc(version, 'prerelease', 'alpha') || 'kungfu-version-unknown'
      );
    case 2:
      return semver.inc(version, 'patch') || 'kungfu-version-unknown';
    case 3: {
      const versionWithoutPre = semver.coerce(version)?.version;
      const incrementedVersion = semver.inc(versionWithoutPre || '', 'patch');
      const finalVersion = incrementedVersion + '-alpha.0';
      return finalVersion;
    }
    case 4: {
      const incrementedVersion = semver.inc(version, 'minor');
      return incrementedVersion ? incrementedVersion + '-alpha.0' : version;
    }
    case 5:
      return semver.inc(version, 'minor') || 'kungfu-version-unknown';
    default:
      return version;
  }
}

function checkUpdateAvailable1(isPrerelease: boolean, incrementType: number) {
  switch (incrementType) {
    case 1:
    case 4:
      if (isPrerelease) {
        return true;
      } else {
        return !!rootPackageJson?.kungfuCraft?.autoUpdate?.checkVersion
          ?.releaseToPre;
      }
    case 2:
    case 5: {
      if (isPrerelease) {
        return !!rootPackageJson?.kungfuCraft?.autoUpdate?.checkVersion
          ?.preToRelease;
      } else {
        return true;
      }
    }
    case 3:
      if (isPrerelease) {
        return true;
      } else {
        return false;
      }
    default:
      return false;
  }
}

async function setUpdaterOption(
  targetVersion: string,
  rawUpdateOption: Writeable<AllPublishOptions>,
  projectName: string,
  version: semver.SemVer,
): Promise<
  | false
  | {
      updaterOption: Writeable<AllPublishOptions>;
      availableVersion: string;
    }
> {
  const updaterOption = { ...rawUpdateOption };
  if (!targetVersion || !updaterOption) return false;

  const artifactPath = `${projectName}/v${version.major}/v${targetVersion}`;
  updaterOption.channel = getChannel(targetVersion.includes('-alpha'));
  let baseUrl = '';

  let ymlUrl = '';
  if (updaterOption.provider === 'generic') {
    baseUrl = updaterOption.url;
    ymlUrl = `${updaterOption.url}/${artifactPath}/${updaterOption.channel}.yml`;
  } else if (updaterOption.provider === 's3') {
    // const s3BaseUrl = updaterOption.region
    //   ? `https://s3.${updaterOption.region}.amazonaws.com/${updaterOption.bucket}`
    //   : `https://s3.amazonaws.com/${updaterOption.bucket}`;
    updaterOption.path = artifactPath;
    //TODO: s3 ymlUrl
    ymlUrl = '';
  }

  const urlPrefix = `${baseUrl}/${projectName}/v${version.major}/v`;

  // // 检查当前版本的URL是否可用
  const isUrlAvailable = await checkUrl(ymlUrl);
  if (isUrlAvailable) {
    const result = await getLastedVersion(
      targetVersion,
      targetVersion.includes('-alpha'),
      1,
      updaterOption,
      urlPrefix,
    );

    if (result) {
      kfLogger.info(
        'Kungfu autoUpdater setFeedURL: ',
        JSON.stringify(result.updaterOption),
      );
      autoUpdater.setFeedURL(result.updaterOption);
      return {
        updaterOption: result.updaterOption,
        availableVersion: result.availableVersion || '',
      };
    } else {
      return false;
    }
  } else {
    return false;
  }
}

async function getLastedVersion(
  availableVersion: string,
  isPrerelease: boolean,
  UpdateType: number,
  updaterOption: Writeable<AllPublishOptions>,
  urlPrefix: string,
): Promise<
  | false
  | {
      updaterOption: Writeable<AllPublishOptions>;
      availableVersion: string;
    }
> {
  if (!checkUpdateAvailable1(isPrerelease, UpdateType)) {
    if (++UpdateType <= 5) {
      kfLogger.info('checkUpdateAvailable1 ', availableVersion, UpdateType);
      return getLastedVersion(
        availableVersion,
        isPrerelease,
        UpdateType,
        updaterOption,
        urlPrefix,
      );
    } else {
      kfLogger.info('checkUpdateAvailable1 ', availableVersion, UpdateType);
      const artifactPath = `${urlPrefix}${availableVersion}`;
      if (updaterOption.provider === 'generic') {
        updaterOption.url = `${artifactPath}`;
      } else if (updaterOption.provider === 's3') {
        updaterOption.path = artifactPath;
      }

      return {
        updaterOption,
        availableVersion: availableVersion || '',
      };
    }
  }
  const targetVersion = updateVersion(availableVersion, UpdateType);
  let ymlUrl = '';
  if (updaterOption.provider === 'generic') {
    const url = `${urlPrefix}${targetVersion}`;
    ymlUrl = `${url}/${getChannel(targetVersion.includes('-alpha'))}.yml`;
  } else if (updaterOption.provider === 's3') {
    ymlUrl = '';
  }
  const isUrlAvailable = await checkUrl(ymlUrl);
  if (isUrlAvailable) {
    kfLogger.info('getLastedVersion ', targetVersion, UpdateType);
    return getLastedVersion(
      targetVersion,
      targetVersion.includes('-alpha'),
      1,
      updaterOption,
      urlPrefix,
    );
  } else {
    if (++UpdateType <= 5) {
      kfLogger.info('getLastedVersion1 ', targetVersion, UpdateType);
      return getLastedVersion(
        availableVersion,
        isPrerelease,
        UpdateType,
        updaterOption,
        urlPrefix,
      );
    } else {
      kfLogger.info('getLastedVersion2 ', targetVersion, UpdateType);
      const artifactPath = `${urlPrefix}${availableVersion}`;
      if (updaterOption.provider === 'generic') {
        updaterOption.url = `${artifactPath}`;
      } else if (updaterOption.provider === 's3') {
        updaterOption.path = artifactPath;
      }
      return {
        updaterOption,
        availableVersion: availableVersion || '',
      };
    }
  }
}

async function checkUrl(url: string): Promise<boolean> {
  try {
    const response = await axios.get(url, { responseType: 'stream' });
    kfLogger.info('checkUrl success', response.status, url);
    // 立即终止下载，不读取数据
    response.data.destroy();
    return true;
  } catch (error) {
    kfLogger.error('checkUrl error', error, url);
    return false;
  }
}

async function handleUpdateKungfu(
  MainWindow: BrowserWindow | null,
  targetVersions: string[] = [],
) {
  kfLogger.info('Kungfu client version: ', app.getVersion());
  kfLogger.info('Kungfu client isPacked: ', app.isPackaged);
  if (!app.isPackaged) return;
  let curErrorBeCalled = false;
  let downloadStarted = false;

  ipcMain.on('auto-update-retry-check-update', async () => {
    kfLogger.info('auto-update-retry-check-update');

    const curVersion = getCurrentLastedVersion(rootPackageJson.version || '');
    targetVersions = getDefaultTargetVersions(version);
    kfLogger.info('retry version', curVersion, targetVersions);
    if (curVersion === '') return;
    version = semver.parse(curVersion as string) as semver.SemVer;
    curTargetVersion = targetVersions.shift();
    if (!curTargetVersion || !rawUpdateOption) return;
    const updaterOptionResult = await setUpdaterOption(
      curTargetVersion,
      rawUpdateOption,
      projectName,
      version,
    );
    if (!updaterOptionResult) return;

    const { updaterOption, availableVersion } = updaterOptionResult;
    if (availableVersion) {
      setUpdateListener(availableVersion);
    }

    kfLogger.info(
      'Kungfu autoUpdater recheck option: ',
      JSON.stringify(updaterOption),
    );
    setupAutoUpdaterListeners(MainWindow, targetVersions);
    autoUpdater.checkForUpdates();
  });

  function setUpdateListener(version: string) {
    if (MainWindow) {
      kfLogger.info('Got lastest version', version);
      foundNewVersion(MainWindow, version);

      ipcMain.on('auto-update-skip-version', (event, version) => {
        saveSkippedVersion(version);
        skipVersion(MainWindow, version);
      });

      ipcMain.on('auto-update-confirm-result', (_, result) => {
        if (result) {
          download();
          downloadStarted = true;
          kfLogger.info('Kungfu autoUpdater start-download');
          startDownloadNewVersion(MainWindow);
        }
      });

      ipcMain.on('auto-update-to-start-download', () => {
        download();
        downloadStarted = true;
        kfLogger.info('Kungfu autoUpdater start-download');
        startDownloadNewVersion(MainWindow);
      });
    }
  }

  function setupAutoUpdaterListeners(
    MainWindow: BrowserWindow | null,
    targetVersions: string[],
  ) {
    autoUpdater.removeAllListeners();

    autoUpdater.on('error', async (error) => {
      kfLogger.error('Kungfu autoUpdater error message: ', error?.message);
      if (MainWindow && downloadStarted) sendUpdatingError(MainWindow, error);
      if (!curErrorBeCalled && !downloadStarted && targetVersions.length) {
        curErrorBeCalled = true;
        await handleUpdateKungfu(MainWindow, targetVersions);
      }
    });

    autoUpdater.on('checking-for-update', () => {
      kfLogger.info('Checking for update');
    });

    autoUpdater.on('update-available', (info) => {
      kfLogger.info('Got a new kungfu client version', JSON.stringify(info));
      if (MainWindow) {
        foundNewVersion(MainWindow, info.version);

        ipcMain.on('auto-update-skip-version', (event, version) => {
          saveSkippedVersion(version);
          skipVersion(MainWindow, version);
        });

        ipcMain.on('auto-update-confirm-result', (_, result) => {
          if (result) {
            download();
            downloadStarted = true;
            kfLogger.info('Kungfu autoUpdater start-download');
            startDownloadNewVersion(MainWindow);
          }
        });

        ipcMain.on('auto-update-to-start-download', () => {
          download();
          downloadStarted = true;
          kfLogger.info('Kungfu autoUpdater start-download');
          startDownloadNewVersion(MainWindow);
        });
      }
    });

    autoUpdater.on('update-not-available', async (info) => {
      kfLogger.info('Current version is up-to-date', JSON.stringify(info));
      if (MainWindow) updateNotAvailable(MainWindow);
      if (targetVersions.length)
        await handleUpdateKungfu(MainWindow, targetVersions);
    });

    autoUpdater.on('update-downloaded', (info) => {
      kfLogger.info('update-downloaded', JSON.stringify(info));
      if (MainWindow) {
        downloadProcessUpdate(MainWindow, 100);
        ipcMain.on('auto-update-quit-and-install', () => {
          if (!MainWindow) return;
          Promise.all([
            reqRecordBeforeQuit(MainWindow),
            killAllBeforeQuit(MainWindow),
          ])
            .catch((err) => {
              kfLogger.error(err);
            })
            .finally(() => {
              delayMilliSeconds(1000).then(() => {
                removeTargetFilesInFolder(
                  KF_HOME,
                  ['.db', '.journal'],
                  ['etc', 'config.db'],
                ).then((results) => {
                  globalStorage.setItem('needClearJournal', true);
                  results.errors.forEach((error) => kfLogger.error(error));
                  delayMilliSeconds(1000).then(() => {
                    autoUpdater.quitAndInstall(false, true);
                    app.exit();
                  });
                });
              });
            });
        });
      }
    });

    autoUpdater.on('download-progress', (progressInfo) => {
      kfLogger.info('Download progress: ', JSON.stringify(progressInfo));
      if (MainWindow) downloadProcessUpdate(MainWindow, progressInfo.percent);
    });
  }

  function configureAutoUpdater() {
    autoUpdater.autoDownload = false;
    autoUpdater.autoInstallOnAppQuit = false;
    autoUpdater.autoRunAppAfterInstall = true;
  }

  const rawUpdateOption = rootPackageJson?.kungfuCraft?.autoUpdate?.update;
  if (!rootPackageJson || !rawUpdateOption) return;

  const projectName = getProjectName(rootPackageJson);
  const curVersion = getCurrentLastedVersion(rootPackageJson.version || '');
  if (curVersion === '') return;
  let version = semver.parse(curVersion as string) as semver.SemVer;

  if (!targetVersions.length) {
    targetVersions = getDefaultTargetVersions(version);
    kfLogger.info('targetVersions.length', targetVersions, version);
  }
  if (!targetVersions.length) return;

  kfLogger.info(
    'Kungfu autoUpdater all target versions: ',
    JSON.stringify(targetVersions),
  );

  let curTargetVersion = targetVersions.shift();
  if (!curTargetVersion) return;

  const updaterOptionResult = await setUpdaterOption(
    curTargetVersion,
    rawUpdateOption,
    projectName,
    version,
  );
  if (!updaterOptionResult) return;

  const { updaterOption, availableVersion } = updaterOptionResult;
  if (availableVersion) {
    setUpdateListener(availableVersion);
  }

  kfLogger.info(
    'Kungfu autoUpdater check option: ',
    JSON.stringify(updaterOption),
  );

  configureAutoUpdater();
  setupAutoUpdaterListeners(MainWindow, targetVersions);
  if (isRendererReady) {
    autoUpdater.checkForUpdates();
  } else {
    ipcMain.on('auto-update-renderer-ready', () => {
      kfLogger.info('auto-update-renderer-ready');
      autoUpdater.checkForUpdates();
      isRendererReady = true;
    });
  }
}

export { handleUpdateKungfu };

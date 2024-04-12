import semver from 'semver';
import path from 'path';
import { app, ipcMain, BrowserWindow, dialog, shell } from 'electron';
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
import VueI18n from '@kungfu-trader/kungfu-js-api/language';

const { t } = VueI18n.global;
const globalStorage = getGlobalStorage();
const rootPackageJson = readRootPackageJsonSync();

autoUpdater.logger = kfLogger;
let isRendererReady = false;

const UpdateVersionTypeEnums = {
  UpdateToNextAlpha: 1, //更新到下一个alpha版本
  UpdateToRelease: 2, //更新到release版本
  UpdateAlphaPatch: 3, //更新patch版本
  UpdateMinorWithAlpha: 4, //更新minor版本
  UpdateToNextRelease: 5, //更新到下一个release版本
};

type UpdateVersionType =
  typeof UpdateVersionTypeEnums[keyof typeof UpdateVersionTypeEnums];

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

const getLatestSkippedVersion = () => {
  const skippedVersions = globalStorage.getItem('skippedVersions');

  if (skippedVersions && skippedVersions.length > 0) {
    return skippedVersions[skippedVersions.length - 1];
  } else {
    return false;
  }
};

const getCurrentLatestVersion = (currentVersion: string) => {
  const skipLatestVersion = getLatestSkippedVersion();
  return skipLatestVersion
    ? semver.gt(currentVersion, skipLatestVersion || '')
      ? currentVersion
      : skipLatestVersion
    : currentVersion;
};

function saveSkippedVersion(version: string) {
  const skippedVersions = globalStorage.getItem('skippedVersions');
  if (skippedVersions && skippedVersions.length > 0) {
    skippedVersions.push(version);
    globalStorage.setItem('skippedVersions', skippedVersions);
  } else {
    globalStorage.setItem('skippedVersions', [version]);
  }
}

const download = debounce(() => {
  autoUpdater.downloadUpdate();
}, 1000);

function getTargetVersion(version: string) {
  const isRelease = !version.includes('-alpha');
  if (isRelease && !isAllowReleaseToAlpha()) {
    return getNextVersionByUpdateType(
      version,
      UpdateVersionTypeEnums.UpdateToRelease,
    );
  } else {
    return getNextVersionByUpdateType(
      version,
      UpdateVersionTypeEnums.UpdateToNextAlpha,
    );
  }
}

function getNextVersionByUpdateType(
  version: string,
  updateType: UpdateVersionType,
): string {
  switch (updateType) {
    case UpdateVersionTypeEnums.UpdateToNextAlpha:
      return (
        semver.inc(version, 'prerelease', 'alpha') || 'kungfu-version-unknown'
      );
    case UpdateVersionTypeEnums.UpdateToRelease:
      return semver.inc(version, 'patch') || 'kungfu-version-unknown';
    case UpdateVersionTypeEnums.UpdateAlphaPatch: {
      const versionWithoutPre = semver.coerce(version)?.version;
      const incrementedVersion = semver.inc(versionWithoutPre || '', 'patch');
      const finalVersion = incrementedVersion + '-alpha.0';
      return finalVersion;
    }
    case UpdateVersionTypeEnums.UpdateMinorWithAlpha: {
      const versionWithoutPre = semver.coerce(version)?.version;
      const incrementedVersion = semver.inc(versionWithoutPre || '', 'minor');
      const finalVersion = incrementedVersion + '-alpha.0';
      return finalVersion;
    }
    case UpdateVersionTypeEnums.UpdateToNextRelease:
      return semver.inc(version, 'minor') || 'kungfu-version-unknown';
    default:
      return version;
  }
}

function isAllowReleaseToAlpha() {
  return !!rootPackageJson?.kungfuCraft?.autoUpdate?.checkVersion
    ?.releaseToAlpha;
}

function isAllowAlphaToRelease() {
  return !!rootPackageJson?.kungfuCraft?.autoUpdate?.checkVersion
    ?.alphaToRelease;
}

function checkUpdateTypeAvailable(
  isPrerelease: boolean,
  updateType: UpdateVersionType,
) {
  switch (updateType) {
    case UpdateVersionTypeEnums.UpdateToNextAlpha:
    case UpdateVersionTypeEnums.UpdateMinorWithAlpha:
      if (isPrerelease) {
        return true;
      } else {
        return isAllowReleaseToAlpha();
      }
    case UpdateVersionTypeEnums.UpdateToRelease:
    case UpdateVersionTypeEnums.UpdateToNextRelease: {
      if (isPrerelease) {
        return isAllowAlphaToRelease();
      } else {
        return true;
      }
    }
    case UpdateVersionTypeEnums.UpdateAlphaPatch:
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
    }
> {
  const updaterOption = { ...rawUpdateOption };
  if (!targetVersion || !updaterOption) return false;

  const artifactPath = `${projectName}/v${version.major}/v${targetVersion}`;
  let baseUrl = '';
  if (updaterOption.provider === 'generic') {
    baseUrl = updaterOption.url;
  } else if (updaterOption.provider === 's3') {
    updaterOption.path = artifactPath;
  }

  const urlPrefix = `${baseUrl}/${projectName}/v${version.major}/v`;

  const result = await getLatestVersion(
    targetVersion,
    targetVersion.includes('-alpha'),
    UpdateVersionTypeEnums.UpdateToNextAlpha,
    updaterOption,
    urlPrefix,
  );

  if (result && result.latestVersion) {
    const availableVersion = result.latestVersion;
    const artifactPath = `${urlPrefix}${availableVersion}`;
    updaterOption.channel = getChannel(availableVersion.includes('-alpha'));
    if (updaterOption.provider === 'generic') {
      updaterOption.url = `${artifactPath}`;
    } else if (updaterOption.provider === 's3') {
      updaterOption.path = artifactPath;
    }
    autoUpdater.setFeedURL(updaterOption);
    return {
      updaterOption,
    };
  } else {
    return false;
  }
}

async function getLatestVersion(
  availableVersion: string,
  isPrerelease: boolean,
  updateVersionType: number,
  updaterOption: Writeable<AllPublishOptions>,
  urlPrefix: string,
): Promise<
  | false
  | {
      latestVersion: string;
    }
> {
  if (!checkUpdateTypeAvailable(isPrerelease, updateVersionType)) {
    if (++updateVersionType <= UpdateVersionTypeEnums.UpdateToNextRelease) {
      return getLatestVersion(
        availableVersion,
        isPrerelease,
        updateVersionType,
        updaterOption,
        urlPrefix,
      );
    } else {
      return {
        latestVersion: availableVersion,
      };
    }
  }
  const targetVersion = getNextVersionByUpdateType(
    availableVersion,
    updateVersionType,
  );
  let ymlUrl = '';
  if (updaterOption.provider === 'generic') {
    const url = `${urlPrefix}${targetVersion}`;
    ymlUrl = `${url}/${getChannel(targetVersion.includes('-alpha'))}.yml`;
  } else if (updaterOption.provider === 's3') {
    ymlUrl = '';
  }
  const isUrlAvailable = await checkUrl(ymlUrl);
  if (isUrlAvailable) {
    return getLatestVersion(
      targetVersion,
      targetVersion.includes('-alpha'),
      UpdateVersionTypeEnums.UpdateToNextAlpha,
      updaterOption,
      urlPrefix,
    );
  } else {
    if (++updateVersionType <= UpdateVersionTypeEnums.UpdateToNextRelease) {
      return getLatestVersion(
        availableVersion,
        isPrerelease,
        updateVersionType,
        updaterOption,
        urlPrefix,
      );
    } else {
      return {
        latestVersion: availableVersion,
      };
    }
  }
}

async function checkUrl(url: string): Promise<boolean> {
  try {
    const response = await axios.get(url, { responseType: 'stream' });
    response.data.destroy();
    return true;
  } catch (error) {
    return false;
  }
}

async function handleUpdateKungfu(MainWindow: BrowserWindow | null) {
  kfLogger.info('Kungfu client version: ', app.getVersion());
  kfLogger.info('Kungfu client isPacked: ', app.isPackaged);
  if (!app.isPackaged) return;
  let targetVersion = '';
  let downloadStarted = false;
  let findLastestVersion = false;
  let isDownloadLatestVersion = false;

  const rawUpdateOption = rootPackageJson?.kungfuCraft?.autoUpdate?.update;
  if (!rootPackageJson || !rawUpdateOption) return;

  const projectName = getProjectName(rootPackageJson);
  const curVersion = getCurrentLatestVersion(rootPackageJson.version || '');
  if (curVersion === '') return;
  const version = semver.parse(curVersion as string) as semver.SemVer;

  targetVersion = getTargetVersion(curVersion);
  if (!targetVersion) return;
  kfLogger.info('Kungfu autoUpdater target version: ', targetVersion);
  function setupAutoUpdaterListeners(MainWindow: BrowserWindow | null) {
    autoUpdater.removeAllListeners();

    autoUpdater.on('error', async (error) => {
      kfLogger.error('Kungfu autoUpdater error message: ', error?.message);
      if (MainWindow && downloadStarted) sendUpdatingError(MainWindow, error);
    });

    autoUpdater.on('checking-for-update', () => {
      kfLogger.info('Checking for update');
    });

    autoUpdater.on('update-available', (info) => {
      kfLogger.info('Got a new kungfu client version', JSON.stringify(info));
      if (MainWindow) {
        if (!isDownloadLatestVersion) {
          foundNewVersion(MainWindow, info.version);
        }

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
                  delayMilliSeconds(1000).then(async () => {
                    const isMacOS = process.platform === 'darwin';
                    const downloadedFilePath = info.downloadedFile;
                    const isZip = downloadedFilePath.endsWith('.zip');
                    if (isMacOS && isZip) {
                      const zipName = downloadedFilePath.substring(
                        downloadedFilePath.lastIndexOf('/') + 1,
                      );

                      const response = dialog.showMessageBoxSync({
                        type: 'info',
                        buttons: [t('open_folder'), t('close')],
                        title: t('install_app'),
                        message: t('unzip_tip', { zipName }),
                        detail: t('open_folder_detail'),
                      });

                      if (response === 0) {
                        shell.showItemInFolder(path.join(downloadedFilePath));
                      }
                    } else {
                      autoUpdater.quitAndInstall(false, true);
                    }
                    app.exit();
                  });
                });
              });
            });
        });
        isDownloadLatestVersion = false;
      }
    });

    autoUpdater.on('download-progress', (progressInfo) => {
      kfLogger.info('Download progress: ', JSON.stringify(progressInfo));
      if (MainWindow) downloadProcessUpdate(MainWindow, progressInfo.percent);
    });
  }

  function setupIpcListeners() {
    ipcMain.on('auto-update-renderer-ready', () => {
      kfLogger.info('auto-update-renderer-ready');
      if (findLastestVersion) {
        autoUpdater.checkForUpdates();
      }
      isRendererReady = true;
    });

    ipcMain.on('auto-update-to-download-latest', async () => {
      if (MainWindow) {
        const curVersion = getCurrentLatestVersion(
          rootPackageJson.version || '',
        );
        if (!curVersion || !rawUpdateOption) return;
        const updaterOptionResult = await setUpdaterOption(
          curVersion,
          rawUpdateOption,
          projectName,
          version,
        );
        if (!updaterOptionResult) return;
        isDownloadLatestVersion = true;

        const { updaterOption } = updaterOptionResult;
        kfLogger.info(
          'Kungfu autoUpdater download latest option: ',
          JSON.stringify(updaterOption),
        );
        setupAutoUpdaterListeners(MainWindow);
        autoUpdater.checkForUpdates();
        download();
        downloadStarted = true;
        kfLogger.info('Kungfu autoUpdater start-download');
        startDownloadNewVersion(MainWindow);
      }
    });

    ipcMain.on('auto-update-retry-check-update', async () => {
      const curVersion = getCurrentLatestVersion(rootPackageJson.version || '');
      let version = semver.parse(curVersion as string) as semver.SemVer;
      targetVersion = getTargetVersion(curVersion);
      if (curVersion === '') return;
      version = semver.parse(curVersion as string) as semver.SemVer;
      if (!targetVersion || !rawUpdateOption) return;
      const updaterOptionResult = await setUpdaterOption(
        targetVersion,
        rawUpdateOption,
        projectName,
        version,
      );
      if (!updaterOptionResult) return;

      const { updaterOption } = updaterOptionResult;

      kfLogger.info(
        'Kungfu autoUpdater recheck option: ',
        JSON.stringify(updaterOption),
      );
      setupAutoUpdaterListeners(MainWindow);
      autoUpdater.checkForUpdates();
    });
  }

  function configureAutoUpdater() {
    autoUpdater.autoDownload = false;
    autoUpdater.autoInstallOnAppQuit = false;
    autoUpdater.autoRunAppAfterInstall = true;
  }

  configureAutoUpdater();
  setupAutoUpdaterListeners(MainWindow);
  setupIpcListeners();

  const updaterOptionResult = await setUpdaterOption(
    targetVersion,
    rawUpdateOption,
    projectName,
    version,
  );

  if (!updaterOptionResult) {
    kfLogger.info('Kungfu autoUpdater no available version to update');
    return;
  }

  const { updaterOption } = updaterOptionResult;
  if (updaterOption) {
    findLastestVersion = true;
  }

  kfLogger.info(
    'Kungfu autoUpdater check option: ',
    JSON.stringify(updaterOption),
  );

  if (findLastestVersion && isRendererReady) {
    autoUpdater.checkForUpdates();
  }
}

function checkToClearSkippedVersionList() {
  const skipLatestVersion = getLatestSkippedVersion();
  if (!skipLatestVersion) return;
  const currentVersion = rootPackageJson?.version;
  if (currentVersion) {
    if (semver.gt(currentVersion, skipLatestVersion)) {
      globalStorage.setItem('skippedVersions', []);
    } else if (currentVersion === skipLatestVersion) {
      globalStorage.setItem('skippedVersions', []);
    }
  } else {
    globalStorage.setItem('skippedVersions', []);
  }
}

checkToClearSkippedVersionList();

export { handleUpdateKungfu };

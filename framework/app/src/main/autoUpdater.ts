import semver from 'semver';
import { app, ipcMain, BrowserWindow } from 'electron';
import { autoUpdater } from 'electron-updater';
import getGlobalStorage from '@kungfu-trader/kungfu-js-api/utils/globalStorage';
import {
  delayMilliSeconds,
  kfLogger,
  debounce,
} from '@kungfu-trader/kungfu-js-api/utils/busiUtils';
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
import {
  RootConfigJSON,
  Writeable,
  AllPublishOptions,
} from '@kungfu-trader/kungfu-js-api/typings/global';
const globalStorage = getGlobalStorage();

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

function setUpdaterOption(
  targetVersion: string,
  rawUpdateOption: Writeable<AllPublishOptions>,
  projectName: string,
  version: semver.SemVer,
) {
  const updaterOption = { ...rawUpdateOption };
  if (!targetVersion || !updaterOption) return false;

  const artifactPath = `${projectName}/v${version.major}/v${targetVersion}`;
  updaterOption.channel = getChannel(targetVersion.includes('-alpha'));
  if (updaterOption.provider === 'generic') {
    updaterOption.url = `${updaterOption.url}/${artifactPath}`;
  } else if (updaterOption.provider === 's3') {
    updaterOption.path = artifactPath;
  }

  autoUpdater.setFeedURL(updaterOption);
  return updaterOption;
}

function handleUpdateKungfu(
  MainWindow: BrowserWindow | null,
  targetVersions: string[] = [],
) {
  kfLogger.info('Kungfu client version: ', app.getVersion());
  kfLogger.info('Kungfu client isPacked: ', app.isPackaged);
  if (!app.isPackaged) return;

  function setupAutoUpdaterListeners(
    MainWindow: BrowserWindow | null,
    targetVersions: string[],
  ) {
    autoUpdater.removeAllListeners();
    let curErrorBeCalled = false;
    let downloadStarted = false;

    autoUpdater.on('error', (error) => {
      kfLogger.error('Kungfu autoUpdater error message: ', error?.message);
      if (MainWindow && downloadStarted) sendUpdatingError(MainWindow, error);
      if (!curErrorBeCalled && !downloadStarted && targetVersions.length) {
        curErrorBeCalled = true;
        handleUpdateKungfu(MainWindow, targetVersions);
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

    autoUpdater.on('update-not-available', (info) => {
      kfLogger.info('Current version is up-to-date', JSON.stringify(info));
      if (MainWindow) updateNotAvailable(MainWindow);
      if (targetVersions.length) handleUpdateKungfu(MainWindow, targetVersions);
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

  const rootPackageJson = readRootPackageJsonSync();
  const rawUpdateOption = rootPackageJson?.kungfuCraft?.autoUpdate?.update;
  if (!rootPackageJson || !rawUpdateOption) return;

  const projectName = getProjectName(rootPackageJson);
  const curVersion = getCurrentLastedVersion(rootPackageJson.version || '');
  if (curVersion === '') return;
  let version = semver.parse(curVersion as string) as semver.SemVer;

  if (!targetVersions.length) {
    targetVersions = getDefaultTargetVersions(version);
  }

  if (!targetVersions.length) return;

  kfLogger.info(
    'Kungfu autoUpdater all target versions: ',
    JSON.stringify(targetVersions),
  );

  let curTargetVersion = targetVersions.shift();
  if (!curTargetVersion) return;

  const updaterOption = setUpdaterOption(
    curTargetVersion,
    rawUpdateOption,
    projectName,
    version,
  );
  if (!updaterOption) return;

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

  ipcMain.on('auto-update-retry-check-update', () => {
    kfLogger.info('auto-update-retry-check-update');

    const curVersion = getCurrentLastedVersion(rootPackageJson.version || '');
    if (curVersion === '') return;
    version = semver.parse(curVersion as string) as semver.SemVer;
    targetVersions = getDefaultTargetVersions(version);
    curTargetVersion = targetVersions.shift();
    if (!curTargetVersion || !rawUpdateOption) return;
    const updaterOption = setUpdaterOption(
      curTargetVersion,
      rawUpdateOption,
      projectName,
      version,
    );
    if (!updaterOption) return;
    kfLogger.info(
      'Kungfu autoUpdater recheck option: ',
      JSON.stringify(updaterOption),
    );
    setupAutoUpdaterListeners(MainWindow, targetVersions);
    autoUpdater.checkForUpdates();
  });
}

export { handleUpdateKungfu };

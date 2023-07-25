import path from 'path';
import dayjs from 'dayjs';

import { addFileSync } from '../utils/fileUtils';
import {
  KF_APP_RUNTIME_DIR,
  KF_HOME_BASE_DIR_RESOLVE,
} from '../config/homePathConfig';

addFileSync('', KF_HOME_BASE_DIR_RESOLVE, 'folder');
export const KF_HOME_BASE_DIR = KF_HOME_BASE_DIR_RESOLVE;

//BASE
export const KF_HOME = path.join(KF_HOME_BASE_DIR, 'home');
addFileSync('', KF_HOME, 'folder');

//RUNTIME
export const KF_RUNTIME_DIR = path.join(KF_HOME, 'runtime');
addFileSync('', KF_RUNTIME_DIR, 'folder');

export const RuntimeChildDirTypes: Array<
  'log' | 'db' | 'journal' | 'resources'
> = ['log', 'db', 'journal', 'resources'];
export const buildRuntimeChildDirByType = (
  type: 'log' | 'db' | 'journal' | 'resources',
) => {
  const targetDir = path.join(KF_RUNTIME_DIR, type);
  addFileSync('', targetDir, 'folder');
  return targetDir;
};

//system
export const LOG_SYSTEM_DIR = path.join(
  buildRuntimeChildDirByType('log'),
  'system',
);
addFileSync('', LOG_SYSTEM_DIR, 'folder');

export const LOG_NODE_DIR = path.join(LOG_SYSTEM_DIR, 'node');
addFileSync('', LOG_NODE_DIR, 'folder');

//log
export const LOG_DIR = path.join(KF_HOME, 'logview');
addFileSync('', LOG_DIR, 'folder');

//archive
export const ARCHIVE_DIR = path.join(KF_HOME, 'archive');
addFileSync('', ARCHIVE_DIR, 'folder');

//================= special item start ==============================

//BASE_DB_DIR strategys, accounts, tasks
export const BASE_DB_DIR = path.join(
  buildRuntimeChildDirByType('db'),
  'system',
  'etc',
  'kungfu',
  'live',
);

//RENDERER_LOG_DIR
export const RENDERER_LOG_DIR = path.join(
  LOG_SYSTEM_DIR,
  'node',
  'renderer-app',
  'live',
);

//================== others start =================================

const production = process.env.NODE_ENV === 'production';

//获取进程日志地址
export const buildProcessLogPath = (processId: string) => {
  const tmk = dayjs().format('YYYYMMDD');
  return path.join(LOG_DIR, tmk, `${processId}.log`);
};

//================== others end ===================================

//================== config & resources start =================================

export const KUNGFU_RESOURCES_DIR = globalThis.__publicResources;

export const KF_CONFIG_DEFAULT_PATH = path.join(
  KUNGFU_RESOURCES_DIR,
  'config',
  'kfConfig.json',
);

export const KF_CONFIG_DIR = path.join(KF_APP_RUNTIME_DIR, 'config');
addFileSync('', KF_CONFIG_DIR, 'folder');

export const KF_CONFIG_PATH = path.join(KF_CONFIG_DIR, 'kfConfig.json');

export const KF_INSTRUMENTS_DEFAULT_PATH = path.join(
  KUNGFU_RESOURCES_DIR,
  'config',
  'defaultInstruments.json',
);

export const KF_INSTRUMENTS_PATH = path.join(
  KF_CONFIG_DIR,
  'defaultInstruments.json',
);

export const KF_SUBSCRIBED_INSTRUMENTS_JSON_PATH = path.join(
  KF_CONFIG_DIR,
  'subscribedInstruments.json',
);
addFileSync('', KF_SUBSCRIBED_INSTRUMENTS_JSON_PATH, 'file');

export const KF_TD_GROUP_JSON_PATH = path.join(KF_CONFIG_DIR, 'tdGroups.json');
addFileSync('', KF_TD_GROUP_JSON_PATH, 'file');

export const KF_SCHEDULE_TASKS_JSON_PATH = path.join(
  KF_CONFIG_DIR,
  'scheduleTasks.json',
);
addFileSync('', KF_SCHEDULE_TASKS_JSON_PATH, 'file');

export const PY_WHL_DIR = path.join(KUNGFU_RESOURCES_DIR, 'python');

//================== config & resources end ===================================

//================== cli start ====================================
// process.env.CLI_DIR = path.dirname(xxxx/dzxy.js);
const staticDevCliDir = path.resolve('..', 'cli', 'dist', 'cli');
export const CLI_PARENT_DIR = production
  ? path.join(globalThis.__kfResourcesPath, 'app', 'dist')
  : path.dirname(process.env.CLI_DIR || staticDevCliDir);
export const CLI_DIR = process.env.CLI_DIR || path.join(CLI_PARENT_DIR, 'cli');
process.env.CLI_DIR = CLI_DIR;

//================== cli end ======================================

//================== kfc start ====================================
const staticDevKfcDir = path.resolve('..', 'core', 'dist', 'kfc');
export const KFC_PARENT_DIR = production
  ? globalThis.__kfResourcesPath
  : path.dirname(process.env.KFC_DIR || staticDevKfcDir);
export const KFC_DIR = process.env.KFC_DIR || path.join(KFC_PARENT_DIR, 'kfc');
process.env.KFC_DIR = KFC_DIR;

export const KFC_EXECUTABLE = process.platform === 'win32' ? 'kfc.exe' : 'kfc';
export const EXTENSION_DIRS: string[] = Array.from(
  new Set(
    production
      ? [
          path.join(globalThis.__kfResourcesPath, 'app', 'kungfu-extensions'),
          ...((process.env.EXTENSION_DIRS || '').split(path.delimiter) || []),
        ]
      : [
          path.resolve(KFC_PARENT_DIR, '..', '..', '..', 'extensions'),
          path.resolve('node_modules', '@kungfu-trader'),
          path.resolve('dist'),
          ...((process.env.EXTENSION_DIRS || '').split(path.delimiter) || []),
        ],
  ),
);

//================== kfc end ======================================

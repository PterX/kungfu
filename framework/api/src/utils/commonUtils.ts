// 此文件内只放 不依赖外部逻辑 的 纯函数
import dayjs from 'dayjs';
import fse from 'fs-extra';
import path from 'path';
import { Observable } from 'rxjs';
import os from 'os';

export const booleanProcessEnv = (
  val: string | boolean | undefined,
): boolean => {
  if (val === undefined) {
    return false;
  }

  if (val === 'null') {
    return false;
  }

  if (val === 'true') {
    return true;
  } else if (val === 'false') {
    return false;
  } else {
    return !!val;
  }
};

export const ifKfDev = () => booleanProcessEnv(process.env.IS_KF_DEV);

export const dealKfNumber = (
  preNumber: bigint | number | undefined | unknown,
): string | number | bigint => {
  if (
    preNumber === undefined ||
    preNumber === null ||
    preNumber === Infinity ||
    preNumber === -Infinity ||
    Number.isNaN(Number(preNumber))
  ) {
    return '--';
  }

  return Number(preNumber) || 0;
};
export const dealAssetPrice = (
  preNumber: bigint | number | undefined | unknown,
  pricePrecision?: number,
): string => {
  const afterNumber = dealKfNumber(preNumber);

  if (afterNumber === '--') {
    return afterNumber;
  }

  return Number(afterNumber).kfToFixed(pricePrecision ?? 4);
};

export const dealKfPrice = (
  preNumber: bigint | number | undefined | null | unknown,
  pricePrecision?: number,
): string => {
  const afterNumber = dealKfNumber(preNumber);

  if (afterNumber === '--') {
    return afterNumber;
  }

  return Number(afterNumber).kfToFixed(pricePrecision ?? 4);
};

export const getIdByKfLocation = (kfLocation: KungfuApi.KfLocation): string => {
  const isLive = kfLocation.mode === 'live';
  switch (kfLocation.category) {
    case 'md':
      return isLive
        ? `${kfLocation.group}`
        : `${kfLocation.group}_${kfLocation.mode}`;
    case 'strategy':
    case 'operator':
      if (kfLocation.group === 'default') {
        return isLive
          ? `${kfLocation.name}`
          : `${kfLocation.name}_${kfLocation.mode}`;
      } else {
        return isLive
          ? `${kfLocation.group}_${kfLocation.name}`
          : `${kfLocation.group}_${kfLocation.name}_${kfLocation.mode}`;
      }
    default:
      return isLive
        ? `${kfLocation.group}_${kfLocation.name}`
        : `${kfLocation.group}_${kfLocation.name}_${kfLocation.mode}`;
  }
};

export const getProcessIdByKfLocation = (
  kfLocation: KungfuApi.KfLocation,
): string => {
  const mode = kfLocation.mode || 'live';
  switch (kfLocation.category) {
    case 'md':
      return `${kfLocation.category}_${kfLocation.group}_${mode}`;
    case 'strategy':
    case 'operator':
      if (kfLocation.group === 'default') {
        return `${kfLocation.category}_${kfLocation.name}_${mode}`;
      } else {
        return `${kfLocation.category}_${kfLocation.group}_${kfLocation.name}_${mode}`;
      }
    case 'system':
      return `${kfLocation.name}_${mode}`;

    default:
      return `${kfLocation.category}_${kfLocation.group}_${kfLocation.name}_${mode}`;
  }
};

export const getMdTdKfLocationByProcessId = (
  processId: string,
): KungfuApi.KfLocation | null => {
  if (processId.indexOf('td_') === 0) {
    if (processId.split('_').length === 4) {
      const [category, group, name, mode] = processId.split('_');
      return {
        category,
        group,
        name,
        mode: mode || 'live',
      };
    }
  } else if (processId.indexOf('md_') === 0) {
    if (processId.split('_').length === 3) {
      const [category, group, mode] = processId.split('_');
      return {
        category,
        group,
        name: group,
        mode: mode || 'live',
      };
    }
  }

  return null;
};

export const getOperatorKfLocationByProcessId = (
  processId: string,
): KungfuApi.KfLocation | null => {
  if (processId.indexOf('operator_') === 0) {
    const splits = processId.split('_');

    if (splits.length === 4) {
      const [category, group, name, mode] = processId.split('_');
      return {
        category,
        group,
        name,
        mode: mode || 'live',
      };
    } else if (splits.length === 3) {
      const [category, name, mode] = processId.split('_');
      return {
        category,
        group: 'default',
        name,
        mode: mode || 'live',
      };
    }
  }

  return null;
};

export const getKfLocationByProcessId = (
  processId: string,
): KungfuApi.KfLocation | null => {
  if (processId.indexOf('td_') === 0 || processId.indexOf('md_') === 0) {
    return getMdTdKfLocationByProcessId(processId);
  } else if (processId.indexOf('operator_') === 0) {
    return getOperatorKfLocationByProcessId(processId);
  } else if (processId.indexOf('strategy_') === 0) {
    return getStrategyKfLocationByProcessId(processId);
  } else if (
    processId.split('_').length === 2 &&
    processId.indexOf('_live') !== -1
  ) {
    return getSystemKfLocationProcessId(processId);
  }

  return null;
};

export const getDefaultSystemProcessName = () => {
  return ['archive', 'master', 'cached', 'ledger', 'dzxy'];
};

export const isExtService = (location: KungfuApi.KfExtraLocation) => {
  const { category, group, name } = location;
  const defaultSystemProcessName = getDefaultSystemProcessName();
  if (
    category === 'system' &&
    group === 'service' &&
    defaultSystemProcessName.indexOf(name) === -1
  )
    return true;
  return false;
};

export const dealLocationUID = (
  watcher: KungfuApi.Watcher | null,
  uid: number,
): string => {
  if (!watcher) {
    return '--';
  }

  const kfLocation = watcher?.getLocation(uid);
  if (!kfLocation) return '';
  return getIdByKfLocation(kfLocation);
};

export const setTimerPromiseTask = (fn: AnyPromiseFunction, interval = 500) => {
  let taskTimer: number | undefined = undefined;
  let clear = false;
  function timerPromiseTask(fn: AnyPromiseFunction, interval = 500) {
    if (taskTimer)
      globalThis.clearTimeout(taskTimer as unknown as NodeJS.Timeout);
    fn().finally(() => {
      if (clear) {
        if (taskTimer)
          globalThis.clearTimeout(taskTimer as unknown as NodeJS.Timeout);
        return;
      }
      taskTimer = +globalThis.setTimeout(() => {
        timerPromiseTask(fn, interval);
      }, interval);
    });
  }
  timerPromiseTask(fn, interval);
  return {
    clearLoop: function () {
      clear = true;
      if (taskTimer != null)
        globalThis.clearTimeout(taskTimer as unknown as NodeJS.Timeout);
    },
  };
};

export const getResultUntilValuable = <T>(
  getter: (...args) => T | false,
  timeout = 5000,
): Promise<T> => {
  return new Promise((resolve, reject) => {
    let count = 0;
    const getterResolved = () => {
      setTimeout(() => {
        try {
          const result = getter();
          if (result) {
            resolve(result);
          } else {
            if (++count * 16 > timeout) {
              reject(new Error('GetResultUntilValuable Timeout'));
            } else {
              getterResolved();
            }
          }
        } catch (err) {
          if (++count * 16 > timeout) {
            reject(err);
          } else {
            getterResolved();
          }
        }
      }, 16);
    };

    getterResolved();
  });
};

export function parseTaskSettingsFromEnv(configSettingsEnv = '[]') {
  let configSettings: KungfuApi.KfConfigItem[] = [];

  try {
    configSettings = JSON.parse(configSettingsEnv) as KungfuApi.KfConfigItem[];
  } catch (err) {
    console.error((<Error>err).message);
  }
  return configSettings;
}

export function getYearMonthDay(delimiter = '-') {
  const now = new Date();
  const year = now.getFullYear();
  const month = now.getMonth() + 1;
  const day = now.getDate();
  const monthStr = month < 10 ? `0${month}` : `${month}`;
  const dayStr = day < 10 ? `0${day}` : `${day}`;
  return `${year}${delimiter}${monthStr}${delimiter}${dayStr}`;
}

export function getHourMinuteSecond(delimiter = ':') {
  const now = new Date();
  const hour = now.getHours();
  const minute = now.getMinutes();
  const second = now.getSeconds();
  return `${hour}${delimiter}${minute}${delimiter}${second}`;
}

export function roundToDecimalPlaces(num: number, precision: number) {
  const multiplier = Math.pow(10, precision);
  return Math.round(num * multiplier) / multiplier;
}

export function countDecimalPlaces(num: number) {
  if (String(num).indexOf('e-') !== -1) {
    return parseInt(String(num).split('e-')[1], 10);
  }
  const normalNum = Number(num).kfToFixed(10);
  const numStr = String(normalNum)
    .replace(/(\.\d*?[1-9])0+$/, '$1')
    .replace(/\.0+$/, '');
  const match = numStr.match(/(?:\.(\d+))?$/);
  if (!match) {
    return 0;
  }
  return match[1] ? match[1].length : 0;
}

//深度克隆obj
export const deepClone = <T>(obj: T): T => {
  if (!obj) return obj;
  return JSON.parse(JSON.stringify(obj));
};

export function isCriticalLog(line: string): boolean {
  if (line.indexOf('critical') !== -1) {
    return true;
  }

  if (line.indexOf('File') !== -1) {
    if (line.indexOf('line') !== -1) {
      return true;
    }
  }

  if (line.indexOf('Traceback') != -1) {
    return true;
  }

  if (line.indexOf(' Error ') != -1) {
    return true;
  }

  if (line.indexOf('Try') != -1) {
    if (line.indexOf('for help') != -1) {
      return true;
    }
  }

  if (line.indexOf('Usage') != -1) {
    return true;
  }

  if (line.indexOf('Failed to execute') != -1) {
    return true;
  }

  if (line.indexOf('KeyboardInterrupt') != -1) {
    return true;
  }

  return false;
}

export const resolveInstrumentValue = (
  type: 'instrument' | 'instruments' | 'instrumentsCsv',
  value: string | string[],
): string[] => {
  if (type === 'instruments' || type === 'instrumentsCsv') {
    return (value || ['']) as string[];
  }
  if (type === 'instrument') {
    return [(value || '') as string];
  } else {
    return [];
  }
};

export const initFormTimePicker = (initValue?: string | string[]) => {
  if (typeof initValue !== 'string' && !Array.isArray(initValue)) return null;

  if (typeof initValue === 'string') {
    let parsedValue: dayjs.Dayjs | null = null;

    if (initValue === 'now') {
      parsedValue = dayjs();
    } else if (/^\d{2}:\d{2}:\d{2}$/.test(initValue)) {
      parsedValue = dayjs(initValue, 'HH:mm:ss');
      if (parsedValue) return parsedValue.format('YYYY-MM-DD HH:mm:ss');
    } else if (/^\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}$/.test(initValue)) {
      parsedValue = dayjs(initValue, 'YYYY-MM-DD HH:mm:ss');
      if (parsedValue) return parsedValue.format('YYYY-MM-DD HH:mm:ss');
    } else if (/^\d{4}-\d{2}-\d{2}$/.test(initValue)) {
      parsedValue = dayjs(initValue, 'YYYY-MM-DD');
      if (parsedValue) return parsedValue.format('YYYY-MM-DD');
    }
  } else if (Array.isArray(initValue)) {
    let parsedStartValue: dayjs.Dayjs | null = null;
    let parsedEndValue: dayjs.Dayjs | null = null;
    const [start, end] = initValue;
    parsedStartValue = dayjs(start, 'YYYY-MM-DD HH:mm:ss');
    parsedEndValue = dayjs(end, 'YYYY-MM-DD HH:mm:ss');

    if (parsedStartValue && parsedEndValue) {
      return [
        parsedStartValue.format('YYYY-MM-DD HH:mm:ss'),
        parsedEndValue.format('YYYY-MM-DD HH:mm:ss'),
      ];
    }
  }

  return null;
};

export const concatPrimaryKey = (arr: string[]) => {
  if (arr.length === 0) return '';

  let result = arr[0];

  if (arr.length > 1) {
    result += '_' + arr[1];
  }

  if (arr.length > 2) {
    for (let i = 2; i < arr.length; i++) {
      result += '-' + arr[i];
    }
  }

  return result;
};

export const getPrimaryKeyFromKfConfigItem = (
  settings: KungfuApi.KfConfigItem[],
): KungfuApi.KfConfigItem[] => {
  return settings.filter((item) => {
    return !!item.primary;
  });
};

export const getPrimaryKeys = (
  settings: KungfuApi.KfConfigItem[],
): string[] => {
  return settings.filter((item) => item.primary).map((item) => item.key);
};

export const sum = (list: number[]): number => {
  if (!list.length) return 0;
  return list.reduce((accumlator, a) => accumlator + +a);
};

export const dealMillionSencond2NanoSecond = (ms: number | bigint) =>
  BigInt(ms) * 1000000n;

export const isHexOrRgbColor = (color: string) =>
  color.startsWith('#') || color.startsWith('rgb') || color.startsWith('rgba');

export const getConfigValue = (kfConfig: KungfuApi.KfConfig) => {
  return JSON.parse(kfConfig.value || '{}');
};

export const debounce = (fn: AnyFunction, delay = 300, immediate = false) => {
  let timeout: number;
  return (...args: Array<unknown>) => {
    if (immediate && !timeout) {
      fn(...args);
    }
    clearTimeout(timeout);

    timeout = +setTimeout(() => {
      fn(...args);
    }, delay);
  };
};

export class KfFixedList<T> {
  list: T[];
  limit: number;

  constructor(limit: number) {
    this.list = [];
    this.limit = limit;
  }

  insert(item: T) {
    if (this.list.length >= this.limit) this.list.shift();
    this.list.push(item);
  }
}

export const isTdStrategyCategory = (category: string): boolean => {
  if (category !== 'td') {
    if (category !== 'strategy') {
      return false;
    }
  }

  return true;
};

export const getSystemKfLocationProcessId = (processId: string) => {
  if (!processId) return null;
  const name = processId.split('_')[0];
  if (name === 'master') {
    return {
      category: 'system',
      group: name,
      name: name,
      mode: 'live',
    };
  } else {
    return {
      category: 'system',
      group: 'service',
      name: name,
      mode: 'live',
    };
  }
};

export const getStrategyKfLocationByProcessId = (
  processId: string,
): KungfuApi.KfLocation | null => {
  if (processId.indexOf('strategy_') === 0) {
    const splits = processId.split('_');

    if (splits.length === 4) {
      const [category, group, name, mode] = processId.split('_');
      return {
        category,
        group,
        name,
        mode: mode || 'live',
      };
    } else if (splits.length === 3) {
      const [category, name, mode] = processId.split('_');
      return {
        category,
        group: 'default',
        name,
        mode: mode || 'live',
      };
    }
  }

  return null;
};

export const dealDateDayOrMonth = (val: number) => {
  return val < 10 ? `0${val}` : `${val}`;
};

export const hidePasswordByLogger = (config: string) => {
  const configCopy = JSON.parse(config);
  Object.keys(configCopy || {}).forEach((key: string) => {
    if (key.includes('password')) {
      configCopy[key] = '******';
    }
  });
  return JSON.stringify(configCopy);
};

export const findSoAndPydFiles = async (
  directory: string,
): Promise<string[]> => {
  const entries = await fse.readdir(directory, { withFileTypes: true });

  const matchingFiles = entries
    .filter(
      (entry) =>
        entry.isFile() &&
        (entry.name.endsWith('.so') || entry.name.endsWith('.pyd')),
    )
    .map((entry) => path.join(directory, entry.name));

  return matchingFiles;
};

export const dealSpaceInPath = (pathname: string) => {
  const normalizePath = path.normalize(pathname);
  return normalizePath.replace(/ /g, ' ');
};

export const resolveTypesInExtConfig = <T extends string>(
  types: T | T[],
): T[] => {
  if (!types) return ['unknown'] as T[];
  const typesResolved = [types].flat().map((type) => type.toLowerCase());
  if (!typesResolved.length) return ['unknown'] as T[];
  return typesResolved as T[];
};

export const mergeObject = (
  targetObj: Record<string, KungfuApi.KfConfigValue>,
  sourceObj: Record<string, KungfuApi.KfConfigValue>,
) => {
  if (typeof targetObj !== 'object' || typeof sourceObj !== 'object')
    return targetObj;

  const allKeys = Array.from(
    new Set([...Object.keys(targetObj), ...Object.keys(sourceObj)]),
  );

  allKeys.forEach((key) => {
    if (key in targetObj) {
      if (key in sourceObj) {
        if (
          typeof targetObj[key] === 'object' &&
          typeof sourceObj[key] === 'object'
        ) {
          targetObj[key] = mergeObject(targetObj[key], sourceObj[key]);
        } else {
          targetObj[key] = sourceObj[key];
        }
      }
    } else {
      if (key in sourceObj) {
        targetObj[key] = sourceObj[key];
      }
    }
  });

  return targetObj;
};

export const buildObjectFromArray = <T>(
  list: Array<T>,
  targetKey: number | string,
  targetValueKey?: number | string,
): Record<string | number, T | T[keyof T] | undefined> => {
  return list.reduce((item1, item2) => {
    const key: number | string | symbol = (item2 || {})[targetKey] || '';
    if (key !== '' && key !== undefined) {
      if (targetValueKey === undefined) {
        item1[key] = item2;
      } else {
        item1[key] = (item2 || {})[targetValueKey];
      }
    }
    return item1;
  }, {} as Record<string | number, T | T[keyof T] | undefined>);
};

export const timestampToTimeString = (
  timestamp: bigint,
  format = 'YYYY-MM-DD HH:mm:ss',
) => {
  const date = new Date(Number(timestamp) / 1000000);
  return dayjs(date).format(format);
};

export const findTargetFromArray = <T>(
  list: Array<T>,
  targetKey: string,
  targetValue: string | number | boolean,
) => {
  const targetList = list.filter(
    (item) => (item || {})[targetKey] === targetValue,
  );
  if (targetList && targetList.length) {
    return targetList[0];
  }
  return null;
};

export const delayMilliSeconds = (miliSeconds: number): Promise<void> => {
  return new Promise((resolve) => {
    const timer = setTimeout(() => {
      resolve();
      clearTimeout(timer);
    }, miliSeconds);
  });
};

export const loopToRunProcess = async <T>(
  promiseFunc: Array<() => Promise<T>>,
  interval = 100,
) => {
  let i = 0;
  const len = promiseFunc.length;
  const resList: (T | Error)[] = [];
  for (i = 0; i < len; i++) {
    const pFunc = promiseFunc[i];
    try {
      const res: T = await pFunc();
      resList.push(res);
    } catch (err: unknown) {
      resList.push(err as Error);
    }

    await delayMilliSeconds(interval);
  }
  return resList;
};

export const buildIfWatcherLiveObservable = (watcher: KungfuApi.Watcher) => {
  let timer; // for ui refresh
  return new Observable<boolean>((sub) => {
    timer = setTimerPromiseTask(async () => {
      if (watcher.isLive()) {
        sub.next(true);
        sub.complete();
        timer && timer.clearLoop();
      }
    }, 1000);
  });
};

export const dealCmdPath = (pathname: string) => {
  if (os.platform() === 'win32') {
    return pathname
      .replace(/\\/g, '/')
      .split('/')
      .map((str) => {
        if (str.includes(' ')) {
          return `"${str}"`;
        }

        return str;
      })
      .join('/');
  }
  return pathname;
};

export const isKfColor = (color: string) => color.startsWith('kf-color');

export const kfConfigItemsToProcessArgs = (
  settings: KungfuApi.KfConfigItem[],
  formState: Record<string, KungfuApi.KfConfigValue>,
): string => {
  return JSON.stringify(
    settings
      .filter((item) => {
        return formState[item.key] !== undefined;
      })
      .reduce((pre, item) => {
        pre[item.key] = formState[item.key];
        return pre;
      }, {} as Record<string, KungfuApi.KfConfigValue>),
  );
};

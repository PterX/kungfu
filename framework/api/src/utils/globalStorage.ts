import fse from 'fs-extra';
import path from 'path';
import { KF_CONFIG_DIR } from '../config/pathConfig';
import { GlobalStorageData } from '../typings/global';

function initStorage<T>(storage: ExtendedGlobalStorage<T>): boolean {
  try {
    const _storage = readStorage<T>(storage);
    if (_storage) {
      storage._storage = _storage;
      return true;
    }
    fse.writeJSONSync(storage._storageUrl, {});
    storage._storage = {} as T & GlobalStorageData;
    return true;
  } catch (e) {
    return false;
  }
}

function updateStorage<T>(storage: ExtendedGlobalStorage<T>): boolean {
  try {
    const _storage = readStorage<T>(storage);
    if (_storage) {
      storage._storage = _storage;
      return true;
    }
    return false;
  } catch (e) {
    return false;
  }
}

function readStorage<T>(
  storage: ExtendedGlobalStorage<T>,
): (T & GlobalStorageData) | false {
  try {
    return fse.readJSONSync(storage._storageUrl) as T & GlobalStorageData;
  } catch (error) {
    return false;
  }
}

function writeStorage<T>(path: string, value: T & GlobalStorageData): boolean {
  try {
    fse.writeJsonSync(path, value);
    return true;
  } catch (e) {
    return false;
  }
}

export interface ExtendedGlobalStorage<T> {
  _storage: T & GlobalStorageData;
  _storageUrl: string;
  setStoragePath: (path: string) => void;
  getStoragePath: () => string;
  getItem: <K extends keyof (T & GlobalStorageData)>(
    key: K,
  ) => (T & GlobalStorageData)[K];
  setItem: <K extends keyof (T & GlobalStorageData)>(
    key: K,
    value: Exclude<(T & GlobalStorageData)[K], undefined | null>,
  ) => boolean;
  getAll: () => T & GlobalStorageData;
  removeItem: (key: keyof (T & GlobalStorageData)) => boolean;
  clear: () => boolean;
}

export function getGlobalStorage<T>(): ExtendedGlobalStorage<T> {
  if (!globalThis.globalStorage) {
    const newGlobalStorage: ExtendedGlobalStorage<T> = {
      _storage: { ifNotFirstRunning: false } as T & GlobalStorageData,
      _storageUrl: path.join(KF_CONFIG_DIR, 'globalStorage.json'),
      setStoragePath: (path: string) => {
        newGlobalStorage._storageUrl = path;
      },
      getStoragePath: () => {
        return newGlobalStorage._storageUrl;
      },
      getItem: <K extends keyof (T & GlobalStorageData)>(key: K) => {
        return newGlobalStorage._storage[key];
      },
      setItem: <K extends keyof (T & GlobalStorageData)>(
        key: K,
        value: Exclude<(T & GlobalStorageData)[K], undefined | null>,
      ) => {
        const _storage = Object.assign({}, newGlobalStorage._storage);
        _storage[key] = value;
        const suc = writeStorage(newGlobalStorage._storageUrl, _storage);
        if (suc) {
          newGlobalStorage._storage = _storage;
          return true;
        }
        return false;
      },
      getAll: () => {
        return newGlobalStorage._storage;
      },
      removeItem: (key: keyof (T & GlobalStorageData)) => {
        const _storage = Object.assign({}, newGlobalStorage._storage);
        delete _storage[key];
        const suc = writeStorage(newGlobalStorage._storageUrl, _storage);
        if (suc) {
          newGlobalStorage._storage = _storage;
          return true;
        }
        return false;
      },
      clear: () => {
        const suc = writeStorage(newGlobalStorage._storageUrl, {});
        if (suc) {
          newGlobalStorage._storage = {} as T & GlobalStorageData;
          return true;
        }
        return false;
      },
    };
    initStorage(newGlobalStorage);
    globalThis.globalStorage = new Proxy(newGlobalStorage, {
      get: (target, key) => {
        updateStorage(target);
        return Reflect.get(target, key);
      },
    });
  }

  return globalThis.globalStorage;
}

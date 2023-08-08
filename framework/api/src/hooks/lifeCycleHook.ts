import { kfLogger } from '@kungfu-trader/kungfu-js-api/utils/busiUtils';

type Callback = (...args) => Promise<void>;
type ClearRegister = () => void;

type RegisterReturnType = ClearRegister | false;

export enum LifeCycleKeys {
  BeforeStopAllProcesses = 'beforeStopAllProcesses',
}

export class LifeCycleHook {
  private CallbacksMapDefaultKey = 'DEFAULT';
  callbacksMap: Record<LifeCycleKeys, Map<string, Array<Callback>>>;

  constructor() {
    this.callbacksMap = {
      [LifeCycleKeys.BeforeStopAllProcesses]: new Map(),
    };
  }

  register(
    lifeCycle: LifeCycleKeys,
    key: string | Callback,
    callback?: Callback,
  ): RegisterReturnType {
    if (typeof key === 'string') {
      if (typeof callback !== 'function') {
        kfLogger.error('LifeCycle hook register callback must be a function');
        return false;
      }
    } else if (typeof key === 'function') {
      callback = key;
      key = this.CallbacksMapDefaultKey;
    }

    const targetMap = this.callbacksMap[lifeCycle];
    if (targetMap.has(key)) {
      const existedCallbacks = targetMap.get(key) as Callback[];
      existedCallbacks.push(callback as Callback);
      targetMap.set(key, existedCallbacks);
    } else {
      targetMap.set(key, [callback as Callback]);
    }

    return () => this.clear(lifeCycle, key as string, callback);
  }

  async trigger(lifeCycle: LifeCycleKeys) {
    const targetMap = this.callbacksMap[lifeCycle];

    if (targetMap.size === 0) return;

    for (const [key, callbacks] of targetMap) {
      try {
        for (const callback of callbacks) {
          await callback();
        }
        kfLogger.warn(
          `LifeCycle '${lifeCycle}' hook: the key named '${key}' trigger succeed`,
        );
      } catch (error) {
        kfLogger.warn(
          `LifeCycle '${lifeCycle}' hook: the key named '${key}' trigger error: \n${error}`,
        );
      }
    }
  }

  clear(lifeCycle: LifeCycleKeys, key: string, callback?: Callback) {
    const targetMap = this.callbacksMap[lifeCycle];

    if (!targetMap.has(key) || targetMap.get(key)?.length === 0) {
      kfLogger.warn(
        `LifeCycle '${lifeCycle}' hook: callbacks for the key named '${key}' is not found`,
      );
      return false;
    }

    if (callback) {
      const existedCallbacks = targetMap.get(key) as Callback[];
      const index = existedCallbacks.findIndex((cb) => callback === cb);
      const clearedCallbacks = existedCallbacks.splice(index, 1);
      targetMap.set(key, clearedCallbacks);
      return true;
    }

    targetMap.set(key, []);
    return true;
  }

  clearAll(): boolean {
    this.callbacksMap = {
      [LifeCycleKeys.BeforeStopAllProcesses]: new Map(),
    };
    return true;
  }
}

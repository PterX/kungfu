import { kfLogger } from '@kungfu-tech/api/utils/logUtils';
import { generateLocationCombinations } from '@kungfu-tech/api/hooks/hookUtils';

export class ResetOptionHook<Method, ValueType> {
  hookName: string;
  hooks: Record<string, Method>;
  constructor(hookName: string) {
    this.hookName = hookName;
    this.hooks = new Proxy(
      {},
      {
        get(target: Record<string, Method>, prop: string) {
          const locationPairs = prop.split('_');
          if (locationPairs.length != 4) {
            kfLogger.warn(`Invalid ${hookName} hook key: ${prop}`);
            return [];
          }
          const [category, group, name, mode] = prop.split('_');
          const originalKeys: [string, string, string, string] = [
            category,
            group,
            name,
            mode,
          ];

          const findMatchingKey = () => {
            for (const key of generateLocationCombinations(originalKeys)) {
              if (target[key]) {
                return target[key];
              }
            }
            return (
              _kfLocation: KungfuApi.DerivedKfLocation,
              value: ValueType,
            ) => Promise.resolve(value);
          };

          return findMatchingKey();
        },

        set(target: Record<string, Method>, prop: string, value: Method) {
          if (Reflect.has(target, prop)) {
            kfLogger.warn(`${hookName} hook ${prop} already exists`);
            return true;
          }

          kfLogger.info(`${hookName} hook ${prop} register success`);
          Reflect.set(target, prop, value);
          return true;
        },
      },
    );
  }

  register(kfLocation: KungfuApi.DerivedKfLocation, method: Method) {
    const { category, group, name, mode } = kfLocation;
    const key = `${category}_${group}_${name}_${mode}`;
    Reflect.set(this.hooks, key, method);
  }

  unregister(kfLocation: KungfuApi.DerivedKfLocation) {
    const { category, group, name, mode } = kfLocation;
    const key = `${category}_${group}_${name}_${mode}`;
    if (Reflect.has(this.hooks, key)) {
      Reflect.deleteProperty(this.hooks, key);
      kfLogger.info(`Unregistered ${this.hookName} hook: ${key}`);
    }
  }

  trigger(
    kfLocation: KungfuApi.DerivedKfLocation,
    options: ValueType,
  ): Promise<ValueType> {
    const { category, group, name, mode } = kfLocation;
    const key = `${category}_${group}_${name}_${mode}`;
    const hook = Reflect.get(this.hooks, key) as unknown as (
      location: typeof kfLocation,
      options: ValueType,
    ) => Promise<ValueType>;

    if (typeof hook === 'function') {
      return hook(kfLocation, options);
    } else {
      return Promise.reject(new Error(`Hook for key ${key} is not a function`));
    }
  }
}

import { Proc } from 'pm2';
import { kfLogger } from '@kungfu-tech/api/utils/logUtils';
import { generateLocationCombinations } from '@kungfu-tech/api/hooks/hookUtils';

export type PreStartProcessMethod = (
  kfLocation: KungfuApi.DerivedKfLocation,
) => Promise<Proc | void>;

export class PreStartProcessHooks {
  hooks: Record<string, PreStartProcessMethod[]>;
  constructor() {
    this.hooks = new Proxy(
      {},
      {
        get(target: Record<string, PreStartProcessMethod[]>, prop: string) {
          const locationPairs = prop.split('_');
          if (locationPairs.length != 4) {
            kfLogger.warn(`Invalid hook key: ${prop}`);
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
            return [];
          };

          return findMatchingKey();
        },

        set(
          target: Record<string, PreStartProcessMethod[]>,
          prop: string,
          value: PreStartProcessMethod,
        ) {
          if (!Reflect.has(target, prop)) {
            Reflect.set(target, prop, []);
          }

          const methods = Reflect.get(target, prop);
          methods.push(value);
          Reflect.set(target, prop, methods);
          kfLogger.info(`PreStartProcess hook ${prop} register success`);
          return true;
        },
      },
    );
  }

  register(
    kfLocation: KungfuApi.DerivedKfLocation,
    method: PreStartProcessMethod,
  ) {
    const { category, group, name, mode } = kfLocation;
    const key = `${category}_${group}_${name}_${mode}`;
    Reflect.set(this.hooks, key, method);
  }

  trigger(kfLocation: KungfuApi.DerivedKfLocation) {
    const { category, group, name, mode } = kfLocation;
    const key = `${category}_${group}_${name}_${mode}`;
    return Promise.all(
      Reflect.get(this.hooks, key).map((method) => method(kfLocation)),
    ).catch((err) => {
      kfLogger.warn(<Error>err);
    });
  }
}

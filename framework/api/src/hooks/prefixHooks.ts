import { FunctionalComponent } from '@vue/runtime-core/dist/runtime-core';
import { AntdIconProps } from '@ant-design/icons-vue/lib/components/AntdIcon';
import { kfLogger } from '@kungfu-tech/api/utils/logUtils';
import { generateLocationCombinations } from '@kungfu-tech/api/hooks/hookUtils';

export interface PrefixProps {
  key: string;
  prefix: string | FunctionalComponent<AntdIconProps>;
  prefixType: 'text' | 'icon';
}

const basePrefixs: Record<string, PrefixProps> = {
  default: {
    key: 'default',
    prefix: '',
    prefixType: 'text',
  },
};

export class PrefixHooks {
  hooks: Record<string, PrefixProps>;

  constructor() {
    this.hooks = new Proxy(basePrefixs, {
      get(target: Record<string, PrefixProps>, prop: string) {
        const locationPairs = prop.split('_');
        if (locationPairs.length != 4) {
          kfLogger.warn(`Invalid hook key: ${prop}`);
          return Reflect.get(target, 'default');
        }
        const [category, group, name, mode] = prop.split('_');
        const originalKeys = [category, group, name, mode] as [
          string,
          string,
          string,
          string,
        ];

        const findMatchingKey = () => {
          for (const key of generateLocationCombinations(originalKeys)) {
            if (target[key]) {
              return target[key];
            }
          }
          return Reflect.get(target, 'default');
        };

        return findMatchingKey();
      },
      set(
        target: Record<string, PrefixProps>,
        prop: string,
        value: PrefixProps,
      ) {
        if (Reflect.has(target, prop)) {
          return true;
        }

        Reflect.set(target, prop, value);
        return true;
      },
    });
  }

  register(kfLocation: KungfuApi.DerivedKfLocation, props: PrefixProps) {
    const { category, group, name, mode } = kfLocation;
    const key = `${category}_${group}_${name}_${mode}`;
    Reflect.set(this.hooks, key, props);
  }

  isRegistered(key: string) {
    return Reflect.has(this.hooks, key);
  }

  trigger(kfLocation: KungfuApi.DerivedKfLocation) {
    const { category, group, name, mode } = kfLocation;
    const key = `${category}_${group}_${name}_${mode}`;
    return this.hooks[key];
  }
}

import { OptionProps } from 'ant-design-vue/lib/select';
import { computed, reactive } from 'vue';

export enum FiltersEnum {
  DEST = 'DEST',
  SOURCE = 'SOURCE',
  MSG_TYPE = 'MSG_TYPE',
}

export const createFiltersEnumMap = <T>(
  initalValue: T | (() => T),
): Record<FiltersEnum, T> => {
  const isFunction = initalValue instanceof Function;
  return {
    [FiltersEnum.DEST]: isFunction ? initalValue() : initalValue,
    [FiltersEnum.SOURCE]: isFunction ? initalValue() : initalValue,
    [FiltersEnum.MSG_TYPE]: isFunction ? initalValue() : initalValue,
  };
};

export const useFrameFilters = () => {
  type OptionItem = {
    label: string;
    value: string;
  };

  type FilterOptionMap = Record<FiltersEnum, Record<string, OptionItem>>;

  type FilterOptionresolvedMap = Record<FiltersEnum, OptionItem[]>;

  const filtersFormState = reactive<Record<FiltersEnum, string[]>>(
    createFiltersEnumMap(() => []),
  );

  const filtersOptions = reactive<FilterOptionMap>(
    createFiltersEnumMap<Record<string, OptionItem>>(() => ({})),
  );

  const filtersOptionsResolved = computed<FilterOptionresolvedMap>(() => {
    return Object.keys(filtersOptions).reduce<FilterOptionresolvedMap>(
      (pre, item) => {
        pre[item] = Object.values(filtersOptions[item]) as OptionItem[];

        return pre;
      },
      {} as FilterOptionresolvedMap,
    );
  });

  const optionSorter = (a: OptionProps, b: OptionProps) => {
    const flagA = Number(/\d+/.test(a.key));
    const flagB = Number(/\d+/.test(b.key));
    if (flagA === flagB) {
      return a.key.localeCompare(b.key);
    } else if (flagA > flagB) {
      return 1;
    } else {
      return -1;
    }
  };

  const _addOption = (filterEnum: FiltersEnum, option: OptionItem) => {
    if (option.value && !(option.value in filtersOptions[filterEnum])) {
      filtersOptions[filterEnum][option.value] = option;
    }
  };

  const addOption = (
    filterEnum: FiltersEnum,
    option: OptionItem | OptionItem[],
  ) => {
    if (Array.isArray(option)) {
      option.forEach((item) => {
        _addOption(filterEnum, item);
      });
    } else {
      _addOption(filterEnum, option);
    }
  };

  return {
    filtersFormState,
    filtersOptions,
    filtersOptionsResolved,
    optionSorter,
    addOption,
  };
};

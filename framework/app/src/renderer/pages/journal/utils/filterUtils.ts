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

  const filtersFormState = reactive<Record<FiltersEnum, string>>(
    createFiltersEnumMap(''),
  );

  const filtersOptions = reactive<FilterOptionMap>(
    createFiltersEnumMap<Record<string, OptionItem>>(() => ({})),
  );

  const filtersOptionsResolved = computed<FilterOptionresolvedMap>(() => {
    return Object.keys(filtersOptions).reduce<FilterOptionresolvedMap>(
      (pre, item) => {
        pre[item] = Object.values(filtersOptions[item]);
        return pre;
      },
      {} as FilterOptionresolvedMap,
    );
  });

  const addOption = (
    filterEnum: FiltersEnum,
    option: OptionItem | OptionItem[],
  ) => {
    if (Array.isArray(option)) {
      option.forEach((item) => {
        if (!(item.value in filtersOptions[filterEnum]))
          filtersOptions[filterEnum][item.value] = item;
      });
    } else {
      if (!(option.value in filtersOptions[filterEnum])) {
        filtersOptions[filterEnum][option.value] = option;
      }
    }
  };

  return {
    filtersFormState,
    filtersOptions,
    filtersOptionsResolved,
    addOption,
  };
};

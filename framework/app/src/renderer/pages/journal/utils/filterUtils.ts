import { OptionProps } from 'ant-design-vue/lib/select';
import { computed, reactive } from 'vue';

export enum FiltersEnum {
  DEST = 'DEST',
  SOURCE = 'SOURCE',
  MSG_TYPE = 'MSG_TYPE',
}

export enum msgTypeRange {
  ACCOUNT_INFO = 0,
  TRADE_RELATED,
  QUERY_RELATED,
  MARKET_RELATED,
  MARKET_SUBSCRIPTION_RELATED,
  OPERATOR_RELATED,
  SYSTEM_RELATED,
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
  // type TreeSelectProps = {
  //   title?: string;
  //   value?: string;
  //   children?: TreeSelectProps[];
  // };

  type FilterOptionMap = Record<FiltersEnum, OptionItem[]>;

  type FilterOptionresolvedMap = Record<FiltersEnum, OptionItem[]>;

  const filtersFormState = reactive<Record<string, string[]>>({
    MSG_TYPE: [],
  });

  const filtersOptions = reactive<FilterOptionMap>(
    createFiltersEnumMap<OptionItem[]>(() => []),
  );

  const filtersOptionsResolved = computed<FilterOptionresolvedMap>(() => {
    console.log('filtersOptions', filtersOptions);
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

  const addFilterOption = (filterEnum: FiltersEnum, options: OptionItem[]) => {
    options.forEach((option) => {
      if (option.value && !(option.value in filtersOptions[filterEnum])) {
        filtersOptions[filterEnum][option.value] = option;
      }
    });
  };

  return {
    filtersFormState,
    filtersOptions,
    filtersOptionsResolved,
    optionSorter,
    addFilterOption,
  };
};

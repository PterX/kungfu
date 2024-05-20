import { computed, onMounted, reactive, ref } from 'vue';
import VueI18n from '@kungfu-trader/kungfu-js-api/language';
import { msgTypes } from './index';
const { t } = VueI18n.global;

export type ChannelRecords = Record<string, [number, number]>;
export type TreeSelectProps = {
  title?: string;
  value?: string | number;
  children?: TreeSelectProps[];
};

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

const initMsgTypeMaps = () => {
  const msgTypesFilterOptions: TreeSelectProps[] = [
    {
      title: t('journalConfig.account_info'),
      value: '0-1',
      children: [],
    },
    {
      title: t('journalConfig.trade_related'),
      value: '0-2',
      children: [],
    },
    {
      title: t('journalConfig.query_related'),
      value: '0-3',
      children: [],
    },
    {
      title: t('journalConfig.market_related'),
      value: '0-4',
      children: [],
    },
    {
      title: t('journalConfig.market_subscription_related'),
      value: '0-5',
      children: [],
    },
    {
      title: t('journalConfig.operator_related'),
      value: '0-6',
      children: [],
    },
    {
      title: t('journalConfig.system_related'),
      value: '0-7',
      children: [],
    },
  ];
  const allMsgTypes = Object.keys(msgTypes).map((key) => +key);

  Object.entries(msgTypes).forEach(([key, value]) => {
    const numericKey = +key;
    let messageTypeRange;

    if (numericKey > 100 && numericKey < 200) {
      messageTypeRange = msgTypeRange.ACCOUNT_INFO;
    } else if (numericKey > 200 && numericKey < 300) {
      messageTypeRange = msgTypeRange.TRADE_RELATED;
    } else if (numericKey > 300 && numericKey < 400) {
      messageTypeRange = msgTypeRange.QUERY_RELATED;
    } else if (numericKey > 400 && numericKey < 500) {
      messageTypeRange = msgTypeRange.MARKET_RELATED;
    } else if (numericKey > 500 && numericKey < 600) {
      messageTypeRange = msgTypeRange.MARKET_SUBSCRIPTION_RELATED;
    } else if (numericKey > 600 && numericKey < 700) {
      messageTypeRange = msgTypeRange.OPERATOR_RELATED;
    } else if (numericKey > 10000) {
      messageTypeRange = msgTypeRange.SYSTEM_RELATED;
    }

    if (
      messageTypeRange !== undefined &&
      msgTypesFilterOptions[messageTypeRange] !== undefined
    ) {
      (
        msgTypesFilterOptions[messageTypeRange].children as TreeSelectProps[]
      ).push({
        title: value,
        value: numericKey,
      });
    }
  });

  return {
    allMsgTypes,
    msgTypesFilterOptions,
  };
};

export const useFrameFilters = (
  read: boolean,
  write: boolean,
  selectedChannels: string[],
  selectedMsgTypes: number[],
  inverseSelectedMsgType: boolean,
) => {
  const formState = reactive<{
    write: boolean;
    read: boolean;
    selectedChannels: string[];
    selectedMsgTypes: number[];
    inverseSelectedMsgType: boolean;
  }>({
    write: true,
    read: true,
    selectedChannels: [],
    selectedMsgTypes: [],
    inverseSelectedMsgType: false,
  });

  const { msgTypesFilterOptions } = initMsgTypeMaps();

  onMounted(() => {
    formState.read = read;
    formState.write = write;
    formState.selectedChannels = selectedChannels;
    formState.selectedMsgTypes = selectedMsgTypes;
    formState.inverseSelectedMsgType = inverseSelectedMsgType;
  });

  return {
    formState,
    msgTypesFilterOptions,
  };
};

export const MIS_TRADIND_MESSAGE_TYPE = 100;
export const MAX_TRADING_MESSAGE_TYPE = 10000;

export const useMsgTypesMap = () => {
  const selectedMsgTypes = ref<number[]>([]);
  const selectedMsgTypesMap = computed(() => {
    return selectedMsgTypes.value.reduce((pre, key) => {
      pre[key] = true;
      return pre;
    }, {} as Record<number, boolean>);
  });

  return {
    selectedMsgTypes,
    selectedMsgTypesMap,
  };
};

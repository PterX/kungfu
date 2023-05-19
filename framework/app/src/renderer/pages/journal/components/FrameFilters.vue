<template>
  <a-form
    ref="formRef"
    class="kf-config-form journal-tool-frame-filters__form"
    :model="filtersFormState"
    :colon="false"
    :scroll-to-first-error="true"
    layout="inline"
  >
    <a-form-item>
      <a-checkbox v-model:checked="read" @change="handleApplyFilters">
        {{ $t('journalConfig.read_event') }}
      </a-checkbox>
      <a-checkbox v-model:checked="write" @change="handleApplyFilters">
        {{ $t('journalConfig.write_event') }}
      </a-checkbox>
    </a-form-item>
    <a-form-item>
      <a-select
        v-model:value="seletedChannels"
        mode="multiple"
        :max-tag-count="2"
        style="width: 396px"
        :placeholder="$t('journalConfig.select_channel')"
        :options="Object.keys(channels).map((item) => ({ value: item }))"
        @blur="handleApplyFilters"
        @deselect="handleApplyFilters"
        :virtual="false"
        >
      ></a-select>
    </a-form-item>
    <a-form-item
      v-for="item in Object.keys(formLabelMap)"
      :key="item"
      :name="item"
      class="kf-form-item__warp"
    >
      <a-tree-select
        ref="treeSelectRef"
        v-model:value="filtersFormState[item]"
        :tree-data="filtersOptions[item]"
        treeNodeFilterProp="title"
        style="width: 596px"
        :max-tag-count="5"
        tree-checkable
        show-search
        :placeholder="$t('journalConfig.selete_msg_type')"
        allow-clear
        @blur="handleApplyFilters"
      >
        <a-select-option
          v-for="option in filtersOptions[item]"
          :key="option.value"
          :value="option.title"
        >
          {{ option.title }}
        </a-select-option>
      </a-tree-select>
    </a-form-item>
  </a-form>
</template>

<script setup lang="ts">
import { computed, onMounted, ref, watch } from 'vue';
import VueI18n from '@kungfu-trader/kungfu-js-api/language';
import { longfist } from '@kungfu-trader/kungfu-js-api/kungfu';
import {
  FiltersEnum,
  msgTypeRange,
  ChannelRecords,
} from '../utils/filterUtils';
import { useFrameFilters } from '../utils/filterUtils';
import { debounce } from '@kungfu-trader/kungfu-js-api/utils/busiUtils';

const { t } = VueI18n.global;

type TreeSelectProps = {
  title?: string;
  value?: string;
  children?: TreeSelectProps[];
};

const filtersOptions = ref<Record<string, TreeSelectProps[]>>({
  MSG_TYPE: [
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
  ],
});

const props = withDefaults(
  defineProps<{
    channels: ChannelRecords;
    selectedChannels: string[];
  }>(),
  {
    channels: () => ({} as ChannelRecords),
    selectedChannels: () => []
  },
);

const emit = defineEmits<{
  (
    e: 'applyFilters',
    frameFiltersMap: Record<string, string[]>,
    read: boolean,
    write: boolean,
    seletedChannels: string[],
  ): void;
}>();

const treeSelectRef = ref();
const read = ref(true);
const write = ref(true);
const seletedChannels = ref<string[]>([]);
const channels = computed<ChannelRecords>(() => props.channels);

onMounted(() => {
  let msg: Record<number, string> = longfist.msgTypes;

  Object.entries(msg).forEach(([key, value]) => {
    let numericKey = Number(key);
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

    if (messageTypeRange !== undefined) {
      let messageObj = {
        title: `${msg[key]}`,
        value: key,
      };

      (
        filtersOptions.value.MSG_TYPE[messageTypeRange]
          .children as TreeSelectProps[]
      ).push(messageObj);
      if (messageTypeRange !== msgTypeRange.SYSTEM_RELATED) {
        filtersFormState.MSG_TYPE.push(key);
      }
    }
  });
});

const formRef = ref();
const formLabelMap = {
  [FiltersEnum.MSG_TYPE]: t('journalConfig.msg_type'),
};

const { filtersFormState } = useFrameFilters();

watch(() => props.selectedChannels, (newValue, oldValue) => {
  if (oldValue.length && !newValue.length) {
    seletedChannels.value = [];
  }
 })

const handleApplyFilters = debounce(() => {
  emit(
    'applyFilters',
    filtersFormState,
    read.value,
    write.value,
    seletedChannels.value,
  );
}, 200);
</script>

<style lang="less">
.ant-form-inline.kf-config-form.journal-tool-frame-filters__form {
  flex: 1;
  justify-content: flex-end;
  flex-wrap: wrap;

  .ant-form-item {
    margin-right: 0px;
    margin-left: 16px;
    margin-bottom: 8px;

    &:last-child {
      margin-bottom: 0px;
    }

    &.kf-form-item__warp {
      margin-right: 0px;

      .ant-select {
        min-width: 160px;
        margin-right: 0;
      }
    }
  }
}
</style>

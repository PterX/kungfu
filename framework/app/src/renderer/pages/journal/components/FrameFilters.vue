<template>
  <a-form
    ref="formRef"
    class="kf-config-form"
    :model="filtersFormState"
    :colon="false"
    :scroll-to-first-error="true"
    layout="inline"
  >
    <a-form-item>
      <a-checkbox v-model:checked="read">
        {{ $t('journalConfig.read_event') }}
      </a-checkbox>
      <a-checkbox v-model:checked="write">
        {{ $t('journalConfig.write_event') }}
      </a-checkbox>
    </a-form-item>
    <a-form-item
      v-for="item in Object.keys(formLabelMap)"
      :key="item"
      :name="item"
      class="kf-form-item__warp"
    >
      <a-tree-select
        v-model:value="filtersFormState[item]"
        :tree-data="filtersOptions[item]"
        treeNodeFilterProp="title"
        style="width: 396px"
        :max-tag-count="7"
        tree-checkable
        show-search
        :placeholder="$t('keyword_input')"
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
import { onMounted, ref, watch } from 'vue';
import VueI18n from '@kungfu-trader/kungfu-js-api/language';
import { longfist } from '@kungfu-trader/kungfu-js-api/kungfu';
import { FiltersEnum, msgTypeRange } from '../utils/filterUtils';
import { useFrameFilters } from '../utils/filterUtils';

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
const emit = defineEmits<{
  (
    e: 'applyFilters',
    frameFiltersMap: Record<string, string[]>,
    read: boolean,
    write: boolean,
  ): void;
}>();
const read = ref(true);
const write = ref(true);

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
watch(
  () => read.value,
  (newVal, oldVal) => {
    if (newVal !== oldVal) {
      handleApplyFilters();
    }
  },
);
watch(
  () => write.value,
  (newVal, oldVal) => {
    if (newVal !== oldVal) {
      handleApplyFilters();
    }
  },
);

const formRef = ref();
const formLabelMap = {
  [FiltersEnum.MSG_TYPE]: t('journalConfig.msg_type'),
};

const { filtersFormState } = useFrameFilters();

const handleApplyFilters = () => {
  emit('applyFilters', filtersFormState, read.value, write.value);
};
</script>

<style lang="less">
.ant-form-inline {
  .ant-form-item {
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

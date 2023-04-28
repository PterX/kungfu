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
      :label="formLabelMap[item]"
    >
      <a-select
        v-model:value="filtersFormState[item]"
        :options="filtersOptions[item]"
        mode="multiple"
        :max-tag-count="2"
        show-search
        :placeholder="$t('keyword_input')"
        allow-clear
      >
        <a-select-option
          v-for="option in filtersOptions[item]"
          :key="option.label"
          :value="option.value"
        >
          {{ option.label }}
        </a-select-option>
      </a-select>
    </a-form-item>

    <a-form-item>
      <a-button @click="handleApplyFilters">
        {{ $t('journalConfig.apply_filters') }}
      </a-button>
    </a-form-item>
  </a-form>
</template>

<script setup lang="ts">
import { ref, watch } from 'vue';
import VueI18n from '@kungfu-trader/kungfu-js-api/language';
import { longfist } from '@kungfu-trader/kungfu-js-api/kungfu';
import { FiltersEnum } from '../utils/filterUtils';
import { useFrameFilters } from '../utils/filterUtils';

const { t } = VueI18n.global;

const props = withDefaults(
  defineProps<{
    locationMap: Record<string, string>;
    currentLocation: KungfuApi.KfLocation | null;
    currentTimeRange: [bigint, bigint];
  }>(),
  {},
);
const emit = defineEmits<{
  (
    e: 'applyFilters',
    frameFiltersMap: Record<FiltersEnum, string[]>,
    read: boolean,
    write: boolean,
  ): void;
}>();
const read = ref(true);
const write = ref(true);

const formRef = ref();
const formLabelMap = {
  [FiltersEnum.MSG_TYPE]: t('journalConfig.msg_type'),
};

const { filtersFormState, filtersOptions } = useFrameFilters();

watch(
  () => props.locationMap,
  () => {
    let msg: Record<number, string> = longfist.msgTypes;
    filtersOptions.MSG_TYPE = Object.entries(msg).map(([key, value]) => ({
      label: value,
      value: key,
    }));
  },
);

const addOption = (
  filterEnum: FiltersEnum,
  options: {
    label: string;
    value: string;
  }[],
  clear = false,
) => {
  if (clear) {
    filtersFormState.DEST = [];
    filtersFormState.SOURCE = [];
    filtersFormState.MSG_TYPE = [];
  }
  if (filterEnum === FiltersEnum.MSG_TYPE) {
    filtersFormState.MSG_TYPE.push(
      ...options.reduce((pre, item) => {
        if (
          !filtersFormState.MSG_TYPE.includes(item.label) &&
          Number(item.value) <= 10000
        ) {
          pre.push(item.label);
        }

        return pre;
      }, [] as string[]),
    );
  }
};
const handleApplyFilters = () => {
  console.log('过滤', filtersFormState, read.value, write.value);
  emit('applyFilters', filtersFormState, read.value, write.value);
};

defineExpose({
  addOption,
});
</script>

<style lang="less">
.kf-form-item__warp {
  .ant-select {
    min-width: 160px;
  }
}
</style>

<template>
  <a-form
    ref="formRef"
    class="kf-config-form"
    :model="filtersFormState"
    :colon="false"
    :scroll-to-first-error="true"
    layout="inline"
  >
    <a-form-item
      v-for="item in Object.keys(formLabelMap)"
      :key="item"
      :name="item"
      class="kf-form-item__warp"
      :label="formLabelMap[item]"
    >
      <a-select
        v-model:value="filtersFormState[item]"
        mode="multiple"
        :max-tag-count="2"
        show-search
        :placeholder="$t('keyword_input')"
        filter-option
        :filter-sort="optionSorter"
        allow-clear
      >
        <a-select-option
          v-for="option in filtersOptionsResolved[item]"
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
import { ref } from 'vue';
import VueI18n from '@kungfu-trader/kungfu-js-api/language';
import { FiltersEnum } from '../utils/filterUtils';
import { useFrameFilters } from '../utils/filterUtils';

const { t } = VueI18n.global;

const emit = defineEmits<{
  (e: 'applyFilters', frameFiltersMap: Record<FiltersEnum, string[]>): void;
}>();

const formRef = ref();
const formLabelMap = {
  [FiltersEnum.SOURCE]: t('journalConfig.source'),
  [FiltersEnum.DEST]: t('journalConfig.dest'),
  [FiltersEnum.MSG_TYPE]: t('journalConfig.msg_type'),
};

const {
  filtersFormState,
  filtersOptionsResolved,
  optionSorter,
  addFilterOption,
} = useFrameFilters();

const addOption = (
  filterEnum: FiltersEnum,
  options: {
    label: string;
    value: string;
  }[],
) => {
  if (filterEnum === FiltersEnum.MSG_TYPE) {
    filtersFormState.MSG_TYPE.push(
      ...options.reduce((pre, item) => {
        if (filtersFormState.MSG_TYPE.indexOf(item.value) === -1) {
          pre.push(item.value);
        }

        return pre;
      }, [] as string[]),
    );
  }

  addFilterOption(filterEnum, options);
};

const handleApplyFilters = () => {
  emit('applyFilters', filtersFormState);
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

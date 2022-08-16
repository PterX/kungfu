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
      :key="formLabelMap[item].key"
      :name="formLabelMap[item].key"
      class="kf-form-item__warp"
      :label="formLabelMap[item].label"
    >
      <a-select
        v-model:value="filtersFormState[formLabelMap[item].key]"
        show-search
        :placeholder="$t('keyword_input')"
        filter-option
        allow-clear
      >
        <a-select-option
          v-for="option in filtersOptionsResolved[item]"
          :key="option.value"
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
  (e: 'applyFilters', frameFiltersMap: Record<FiltersEnum, string>): void;
}>();

const formRef = ref();
const formLabelMap = {
  [FiltersEnum.DEST]: {
    label: t('journalConfig.dest'),
    key: FiltersEnum.DEST,
  },
  [FiltersEnum.SOURCE]: {
    label: t('journalConfig.source'),
    key: FiltersEnum.SOURCE,
  },
  [FiltersEnum.MSG_TYPE]: {
    label: t('journalConfig.msg_type'),
    key: FiltersEnum.MSG_TYPE,
  },
};

const { filtersFormState, filtersOptionsResolved, addOption } =
  useFrameFilters();

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
    min-width: 120px;
  }
}
</style>

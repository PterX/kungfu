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
        :options="filtersOptions[item]"
        mode="multiple"
        :max-tag-count="2"
        show-search
        :placeholder="$t('keyword_input')"
   
        allow-clear
      >
     
        <!-- <a-select-option
          v-for="option in filtersOptions[item]"
          :key="option.label"
          :value="option.value"
        >
          {{ option.label }}
        </a-select-option> -->
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
const options3 = ref([
      
        {label: 'td/sim/1/live', value: '2764285673'},
        {label: 'system/service/cached/live', value: '2999745424'}
]);
    console.log(options3.value)
const optionsMap = ref<string[]>([]);

const {
  filtersFormState,
  filtersOptions,
  // optionSorter,
  // addFilterOption,
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
        if (!filtersFormState.MSG_TYPE.includes(item.label)) {
          pre.push(item.label);
        }

        return pre;
      }, [] as string[]),
    );
    //options[0]push进filtersOptions.MSG_TYPE中，如果已经存在相同元素不要添加
    if (!filtersOptions.MSG_TYPE) {
      optionsMap.value = [];
    } else if (
      !optionsMap.value.includes(options[0].value) &&
      options[0].label !== undefined &&
      options[0].value !== undefined
    ) {
      optionsMap.value.push(options[0].value);
      filtersOptions.MSG_TYPE.push(options[0]);
    }
  }
  if (filterEnum === FiltersEnum.SOURCE) {
    filtersFormState.SOURCE.push(
      ...options.reduce((pre, item) => {
        if (!filtersFormState.SOURCE.includes(item.label)) {
          pre.push(item.label);
        }

        return pre;
      }, [] as string[]),
    );
    if (!filtersOptions.SOURCE) {
      optionsMap.value = [];
    } else if (
      !optionsMap.value.includes(options[0].value) &&
      options[0].label !== undefined &&
      options[0].value !== undefined
    ) {
      optionsMap.value.push(options[0].value);
      filtersOptions.SOURCE.push(options[0]);
    }
  }

  if (filterEnum === FiltersEnum.DEST) {
    filtersFormState.DEST.push(
      ...options.reduce((pre, item) => {
        if (!filtersFormState.DEST.includes(item.label)) {
          pre.push(item.label);
        }

        return pre;
      }, [] as string[]),
    );
    if (!filtersOptions.DEST) {
      optionsMap.value = [];
    } else if (
      !optionsMap.value.includes(options[0].value) &&
      options[0].label !== undefined &&
      options[0].value !== undefined
    ) {
      optionsMap.value.push(options[0].value);
      filtersOptions.DEST.push(options[0]);
    }
  }
  // filtersOptions.MSG_TYPE = filtersFormState.MSG_TYPE.map(
  //   (item) => ({
  //     label: item,
  //     value: item,
  //   }),
  // );
  // filtersOptions.SOURCE = filtersFormState.SOURCE.map((item) => ({
  //   label: item,
  //   value: item,
  // }));
  // filtersOptions.DEST = filtersFormState.DEST.map((item) => ({
  //   label: item,
  //   value: item,
  // }));

  console.log(
    'filter',
    filterEnum,
    options,
    filtersFormState,
    filtersOptions,
  );
  // addFilterOption(filterEnum, options);
};

// const handleFilterOption = (
//   inputValue: string,
//   option: {
//     key: string;
//     value: string;
//   },
// ) => {
//   const reg = new RegExp(`.*${inputValue}.*`, 'i');
//   return reg.test(option.key);
// };

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

<template>
  <div class="kf-time-slider">
    <div class="kf-time-slider-time">
      {{ startAndEndTimeStr[0] }}
    </div>
    <a-slider
      v-model:value="currentTimeRange"
      :min="limitTimeRange[0]"
      :max="limitTimeRange[1]"
      :step="step"
      range
      :tip-formatter="tipFormatter"
      @after-change="onAfterChange"
    />
    <div class="kf-time-slider-time">
      {{ startAndEndTimeStr[1] }}
    </div>
  </div>
</template>

<script lang="ts" setup>
import { dealKfTime } from '@kungfu-trader/kungfu-js-api/kungfu';
import { computed, reactive } from 'vue';

const props = withDefaults(
  defineProps<{
    timeRange: [number, number];
    limitTimeRange: [number, number];
    step: number;
  }>(),
  {
    step: 60,
  },
);

defineEmits<{
  (e: 'update:timeRange', value: [number, number]): void;
}>();

const currentTimeRange = reactive<[number, number]>([
  props.timeRange[0],
  props.timeRange[1],
]);

const startAndEndTimeStr = computed(() => {
  return [
    dealKfTime(BigInt(currentTimeRange[0])),
    dealKfTime(BigInt(currentTimeRange[1])),
  ];
});

const tipFormatter = (num: number) => {
  return dealKfTime(BigInt(num));
};

const onAfterChange = (value) => {
  console.log(value);
};
</script>

<style lang="less">
.kf-time-slider {
  display: flex;
  align-items: center;
  justify-content: space-between;

  .kf-time-slider-time {
    width: 100px;
    flex: 0 0 100px;
  }
}
</style>

<template>
  <div class="kf-time-slider__wrap">
    <div class="kf-time-slider-time">
      {{ startAndEndTimeStr[0] }}
    </div>
    <a-slider
      :ref="slider"
      v-model:value="currentTimeRange"
      class="kf-time-slider"
      :min="limitRangeResolved[0]"
      :max="limitRangeResolved[1]"
      :step="nano2millionSecond(step)"
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
import { computed, ref, watch } from 'vue';

const props = withDefaults(
  defineProps<{
    timeRange: [bigint, bigint]; // 当前只支持纳秒级别的时间
    limitTimeRange: [bigint, bigint]; // 当前只支持纳秒级别的时间
    step: number;
  }>(),
  {
    step: 10000000, // step 为纳秒级别， 默认为10毫秒
  },
);

const emit = defineEmits<{
  (e: 'update:timeRange', value: [bigint, bigint]): void;
}>();

const slider = ref();

const SCALE = 1000000;
const BIGINT_SCALE = BigInt(SCALE);

const nano2millionSecond = (number: bigint | number) => {
  if (typeof number === 'bigint') {
    return Number(number / BIGINT_SCALE);
  } else {
    return number / SCALE;
  }
};

const million2nanoSecond = (number: number) => {
  return BigInt(number * SCALE);
};

const currentTimeRange = ref<[number, number]>([
  nano2millionSecond(props.timeRange[0]),
  nano2millionSecond(props.timeRange[1]),
]);

watch(
  () => props.timeRange,
  (newRange) => {
    currentTimeRange.value = [
      nano2millionSecond(newRange[0]),
      nano2millionSecond(newRange[1]),
    ];
  },
);

const limitRangeResolved = computed(() => {
  const resolvedRange = [
    nano2millionSecond(props.limitTimeRange[0]),
    nano2millionSecond(props.limitTimeRange[1]),
  ];

  return resolvedRange;
});

const startAndEndTimeStr = computed(() => {
  return [dealKfTime(props.timeRange[0]), dealKfTime(props.timeRange[1])];
});

// const marks = computed(() => {
//   return {
//     [currentTimeRange.value[0]]: startAndEndTimeStr.value[0],
//     [currentTimeRange.value[1]]: startAndEndTimeStr.value[1],
//   };
// });

const tipFormatter = (num: number) => {
  return dealKfTime(BigInt(num * SCALE));
};

const dealUpdateTime = (time: number) => {
  if (time === limitRangeResolved.value[0]) return props.limitTimeRange[0];
  if (time === limitRangeResolved.value[1]) return props.limitTimeRange[1];
  return million2nanoSecond(time);
};

const onAfterChange = (value: [number, number]) => {
  if (
    value[0] === nano2millionSecond(props.timeRange[0]) &&
    value[1] === nano2millionSecond(props.timeRange[1])
  )
    return;

  emit('update:timeRange', [
    dealUpdateTime(value[0]),
    dealUpdateTime(value[1]),
  ]);
};
</script>

<style lang="less">
.kf-time-slider__wrap {
  display: flex;
  align-items: center;
  justify-content: space-between;

  .kf-time-slider-time {
    font-size: 14px;
    width: 100px;
    margin: 0 16px;
    flex: 0 0 100px;
  }

  .kf-time-slider {
    flex: 1;
  }
}
</style>

<template>
  <div class="kf-time-slider__wrap">
    <backward-outlined
      class="forward-icon"
      @click="handleTimeBack()"
    ></backward-outlined>
    <div class="kf-time-slider-time">
      <span class="kf-time-slider-text" style="text-align: end">
        {{ timeStrs[0] }}
      </span>
    </div>
    <a-slider
      ref="slider"
      v-model:value="curTime"
      v-dragging="{ onMouseDown: handleMouseDown, onMouseUp: handleMouseUp }"
      class="kf-time-slider"
      :class="{
        'kf-time-slider-handler-focus-1': false,
        'kf-time-slider-handler-focus-2': true,
      }"
      :tooltip-visible="toolTipVisable"
      :min="nano2millionSecond(props.beginTime)"
      :max="maxTime"
      :step="nano2millionSecond(step)"
      :tip-formatter="tipFormatter"
      @after-change="onAfterChange"
      @mousedown="handleMouseDown"
      @mouseup="handleMouseUp"
    />
    <div class="kf-time-slider-time">
      <span class="kf-time-slider-text" style="text-align: start">
        {{ timeStrs[1] }}
      </span>
    </div>
    <forward-outlined
      class="forward-icon"
      @click="handleTimeForward()"
    ></forward-outlined>
  </div>
</template>

<script lang="ts" setup>
import { computed, nextTick, onMounted, onUnmounted, ref, watch } from 'vue';

import { dealKfTime } from '@kungfu-trader/kungfu-js-api/kungfu';
import { ForwardOutlined, BackwardOutlined } from '@ant-design/icons-vue';
import { SessionStatusEnum } from '@kungfu-trader/kungfu-js-api/typings/enums';

const props = withDefaults(
  defineProps<{
    currentSession: KungfuApi.SessionResolved | null;
    currentTime: bigint; // 当前只支持纳秒级别的时间
    nowTime: bigint;
    beginTime: bigint;
    endTime: bigint;
    step: number;
    stick: boolean;
  }>(),
  {
    step: 10000000, // step 为纳秒级别， 默认为10毫秒
  },
);

const emit = defineEmits<{
  (e: 'update:currentTime', value: bigint): void;
}>();
const toolTipVisable = ref(false);
const SCALE = 1000000;
const BIGINT_SCALE = BigInt(SCALE);
const TEN_SECOND = BigInt(10000000000);
const isDragging = ref<boolean>(false);
const slider = ref();

// const minTime = computed(() => {
//   nano2millionSecond(props.beginTime);
// });
const maxTime = computed(() => {
  if (props.currentSession?.status === SessionStatusEnum.Finished) {
    return nano2millionSecond(props.endTime);
  } else {
    return nano2millionSecond(props.nowTime);
  }
});

onMounted(() => {
  window.addEventListener('mouseup', globalMouseUp);
  nextTick(() => {
    setTimeout(() => {
      toolTipVisable.value = true;
    }, 2000);
  });
});

onUnmounted(() => {
  window.removeEventListener('mouseup', globalMouseUp);
});

const handleMouseDown = () => {
  isDragging.value = true;
};

const handleMouseUp = () => {
  isDragging.value = false;
};

const globalMouseUp = () => {
  if (isDragging.value) {
    isDragging.value = false;
  }
};

const handleTimeBack = () => {
  if (props.currentTime - TEN_SECOND < props.beginTime) {
    curTime.value = nano2millionSecond(props.beginTime);
    onAfterChange(curTime.value);
    return;
  }
  curTime.value = nano2millionSecond(props.currentTime - TEN_SECOND);
  onAfterChange(curTime.value);
};
const handleTimeForward = () => {
  if (props.currentSession?.status === SessionStatusEnum.Finished) {
    if (props.currentTime + TEN_SECOND > props.endTime) {
      curTime.value = nano2millionSecond(props.endTime);
      onAfterChange(curTime.value);
      return;
    }
  } else {
    if (props.currentTime + TEN_SECOND > props.nowTime) {
      curTime.value = nano2millionSecond(props.nowTime);
      onAfterChange(curTime.value);
      return;
    }
  }

  curTime.value = nano2millionSecond(props.currentTime + TEN_SECOND);
  onAfterChange(curTime.value);
};
const nano2millionSecond = (number: bigint | number) => {
  if (typeof number === 'bigint') {
    return Number(number / BIGINT_SCALE);
  } else {
    return number / SCALE;
  }
};

const kfTimeCached = new Map();
const customDealKftime = (time: bigint) => {
  if (!kfTimeCached.has(time)) kfTimeCached.set(time, dealKfTime(time));
  return kfTimeCached.get(time);
};

const million2nanoSecond = (number: number) => {
  return BigInt(number * SCALE);
};

const curTime = ref(nano2millionSecond(props.nowTime));

const timeStrs = ref([
  customDealKftime(props.beginTime),
  customDealKftime(props.nowTime),
]);

watch(
  () => props.nowTime,
  () => {
    timeStrs.value = [
      customDealKftime(props.beginTime),
      customDealKftime(props.endTime || props.nowTime),
    ];
  },
);
watch(
  () => props.beginTime,
  () => {
    timeStrs.value = [
      customDealKftime(props.beginTime),
      customDealKftime(props.endTime || props.nowTime),
    ];
  },
);

watch(
  () => props.currentTime,
  () => {
    curTime.value = nano2millionSecond(props.currentTime);
  },
);

const tipFormatter = (num: number) => {
  console.log(
    'step',
    num,
    props.step,
    nano2millionSecond(props.step),
    num < maxTime.value,
  );
  return dealKfTime(BigInt(num * SCALE));
};

const dealUpdateTime = (time: number) => {
  return million2nanoSecond(time);
};

const onAfterChange = (value: number) => {
  emit('update:currentTime', dealUpdateTime(value));
};
</script>

<style lang="less">
:deep(.ant-slider-track .ant-slider-step) {
  border-color: #fff !important;
  color: #fff !important;
  background-color: #fff !important;
}
.kf-time-slider__wrap {
  display: flex;
  align-items: center;
  justify-content: space-between;

  .kf-time-slider-time {
    width: 100px;
    margin: 0 16px;
    flex: 0 0 100px;
    font-size: 14px;

    .ant-input-group-compact {
      display: flex;

      input {
        width: 112px;
      }

      button {
        width: 24px;
      }
    }

    .kf-time-slider-text {
      display: block;
      width: 100%;
    }
  }

  .kf-time-slider {
    min-width: 360px;
    flex: 1;
  }

  .kf-time-slider-handler-focus-1 {
    .ant-slider-handle-1 {
      border-color: #faad14;
    }
  }

  .kf-time-slider-handler-focus-2 {
    .ant-slider-handle-2 {
      border-color: #faad14;
    }
  }

  .forward-icon {
    font-size: 18px;
    color: #ffffffd9;
    transition: color 0.3s;
  }

  .forward-icon:hover {
    color: #faad14;
  }
}
</style>

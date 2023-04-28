<template>
  <div class="kf-time-slider__wrap">
    <!-- <div class="arrow arrow-left" @click="handleTimeBack()"></div> -->
    <backward-outlined
      class="forward-icon"
      @click="handleTimeBack()"
    ></backward-outlined>
    <div class="kf-time-slider-time">
      <a-input-group v-if="timeInputData[0].inputting" compact>
        <!-- <a-input
          v-model:value="timeInputData[0].value"
          @press-enter="handleConfirmTimeInput(0)"
        />
        <a-button type="normal" @click="resetInputData(0)">
          <template #icon>
            <close-outlined />
          </template>
        </a-button>
        <a-button type="primary" @click="handleConfirmTimeInput(0)">
          <template #icon>
            <check-outlined />
          </template>
        </a-button> -->
      </a-input-group>
      <span v-else class="kf-time-slider-text" style="text-align: end">
        {{ timeStrs[0] }}
      </span>
    </div>
    <!-- <a-slider
      ref="slider"
      v-model:value="currentTimeRange"
      v-dragging="{ onMouseDown: handleMouseDown, onMouseUp: handleMouseUp }"
      class="kf-time-slider"
      :class="{
        'kf-time-slider-handler-focus-1': sticking[0],
        'kf-time-slider-handler-focus-2': sticking[1],
      }"
      :min="limitRangeResolved[0]"
      :max="limitRangeResolved[1]"
      :step="nano2millionSecond(step)"
      range
      :tip-formatter="tipFormatter"
      @after-change="onAfterChange"
      @mousedown="handleMouseDown"
      @mouseup="handleMouseUp"
    /> -->
    <a-slider
      ref="slider"
      v-model:value="curTime"
      v-dragging="{ onMouseDown: handleMouseDown, onMouseUp: handleMouseUp }"
      class="kf-time-slider"
      :class="{
        'kf-time-slider-handler-focus-1': false,
        'kf-time-slider-handler-focus-2': true,
      }"
      :min="nano2millionSecond(props.beginTime)"
      :max="nano2millionSecond(props.nowTime)"
      :step="nano2millionSecond(step)"
      :tip-formatter="tipFormatter"
      @after-change="onAfterChange"
      @mousedown="handleMouseDown"
      @mouseup="handleMouseUp"
    />
    <div class="kf-time-slider-time">
      <a-input-group v-if="timeInputData[1].inputting" compact>
        <a-input
          v-model:value="timeInputData[1].value"
          @press-enter="handleConfirmTimeInput(0)"
        />
        <a-button type="normal" @click="resetInputData(1)">
          <template #icon>
            <close-outlined />
          </template>
        </a-button>
        <a-button type="primary" @click="handleConfirmTimeInput(1)">
          <template #icon>
            <check-outlined />
          </template>
        </a-button>
      </a-input-group>
      <span
        v-else
        class="kf-time-slider-text"
        style="text-align: start"
        @dblclick="handleDbClickTimeText(1)"
      >
        {{ timeStrs[1] }}
      </span>
    </div>
    <!-- <div class="arrow arrow-right" @click="handleTimeForward()"></div> -->
    <forward-outlined
      class="forward-icon"
      @click="handleTimeForward()"
    ></forward-outlined>
  </div>
</template>

<script lang="ts" setup>
import { onMounted, onUnmounted, ref, watch } from 'vue';

import { CheckOutlined, CloseOutlined } from '@ant-design/icons-vue';
import { dealKfTime } from '@kungfu-trader/kungfu-js-api/kungfu';
import VueI18n from '@kungfu-trader/kungfu-js-api/language';
import { messagePrompt } from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/uiUtils';
import { ForwardOutlined, BackwardOutlined } from '@ant-design/icons-vue';

import dayjs from 'dayjs';

const { t } = VueI18n.global;

type DoubleArray<T> = [T, T];

//  q:timeRage是从哪里来的？  从哪里传递过来的？
//  a: 从父组件传递过来的，父组件是Journal.vue
const props = withDefaults(
  defineProps<{
    currentTime: bigint; // 当前只支持纳秒级别的时间
    // limitTimeRange: DoubleArray<bigint>;
    nowTime: bigint;
    beginTime: bigint;
    endTime: bigint;
    // isTimeContinue: boolean;
    nolRange: DoubleArray<bigint>;
    step: number;
    stick: boolean;
  }>(),
  {
    step: 10000000, // step 为纳秒级别， 默认为10毫秒
  },
);

const emit = defineEmits<{
  (e: 'update:currentTime', value: bigint): void;
  // (e: 'timeContinueUpdate', value: boolean): void;
}>();
const SCALE = 1000000;
const BIGINT_SCALE = BigInt(SCALE);
//10秒是多少纳秒
const TEN_SECOND = BigInt(10 * SCALE);
const goStep = BigInt(10000000000);

const isDragging = ref<boolean>(false);
const slider = ref();
// const sticking = ref<DoubleArray<boolean>>([
//   props.currentTime === props.limitTimeRange[0],
//   props.currentTime === props.limitTimeRange[0],
// ]);
const timeInputData = ref<DoubleArray<{ inputting: boolean; value: string }>>([
  { inputting: false, value: '' },
  { inputting: false, value: '' },
]);

onMounted(() => {
  window.addEventListener('mouseup', globalMouseUp);
});

onUnmounted(() => {
  window.removeEventListener('mouseup', globalMouseUp);
});
// const onSingleSliderChange = (value: number) => {
//   if (value === nano2millionSecond(props.currentTime)) return;
//   console.log('onSingleSliderChange', currentTimeRange, value);

//   if (props.stick) {
//     sticking.value[1] = value === limitRangeResolved.value[1];
//   }
//   emit('update:currentTime', [
//     dealUpdateTime(currentTimeRange.value[0]),
//     dealUpdateTime(value),
//   ]);
// };

// const time = BigInt(new Date().getTime()) * 1000000n;

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

function ff(time) {
  return dayjs
    .unix(Number(BigInt(time) / BigInt(1e9)))
    .format('YYYY-MM-DD HH:mm:ss');
}
const handleTimeBack = () => {
  console.log(
    'handleTimeBack',
    ff(props.currentTime),
    ff(props.currentTime - TEN_SECOND),
  );
  curTime.value = nano2millionSecond(props.currentTime - TEN_SECOND);
  onAfterChange(curTime.value);
};
const handleTimeForward = () => {
  console.log(
    'handleTimeForward',
    ff(props.currentTime),
    ff(props.currentTime - TEN_SECOND),
  );
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

const str2millionTime = (timeStr: string) => {
  // timeStr = timeStr.replaceAll(' ', '');
  return new Promise<number>((resolve, reject) => {
    const timeRegx = /(\d{2}):(\d{2}):(\d{2})\.?(\d{3})?/;
    if (!timeRegx.test(timeStr))
      reject(t('journalConfig.input_time_format_error'));
    const [h, m, s, ms] = timeStr.match(timeRegx)?.slice(1) as string[];
    const date = new Date();
    date.setHours(+h);
    date.setMinutes(+m);
    date.setSeconds(+s);
    date.setMilliseconds(+ms || 0);
    resolve(date.getTime());
  });
};

const million2nanoSecond = (number: number) => {
  return BigInt(number * SCALE);
};

const curTime = ref(nano2millionSecond(props.nowTime));

// const limitRangeResolved = ref<DoubleArray<number>>([
//   nano2millionSecond(props.limitTimeRange[0]),
//   nano2millionSecond(props.limitTimeRange[1]),
// ]);

// watch(
//   () => props.nolRange,
//   () => {
//     if (props.isTimeContinue) {
//       console.log('llllll', {
//         curTime: curTime.value,
//         nolRange: props.nolRange,
//       });
//       onAfterChange(nano2millionSecond(props.nolRange[1]));
//     }
//   },
// );
// watch(
//   () => currentTimeRange.value,
//   () => {
//     console.log('externalUpdate', props.isTimeContinue);
//     emit('externalUpdate', false);
//   },
// );

// watch(
//   () => props.limitTimeRange,
//   (newLimitRange) => {
//     limitRangeResolved.value = [
//       nano2millionSecond(newLimitRange[0]),
//       nano2millionSecond(newLimitRange[1]),
//     ];
//   },
// );
const timeStrs = ref([
  customDealKftime(props.beginTime),
  customDealKftime(props.nowTime),
]);

watch(
  () => props.nowTime,
  () => {
    // console.log(
    //   'nowTime',
    //   props.currentTime,
    //   curTime.value,
    //   props.isTimeContinue,
    // );
    // if (props.isTimeContinue) {
    //   timeStrs.value = [
    //     customDealKftime(props.beginTime),
    //     customDealKftime(props.endTime || props.nowTime),
    //   ];
    // } else {
    //   timeStrs.value = [
    //     customDealKftime(props.beginTime),
    //     customDealKftime(props.currentTime),
    //   ];
    // }
    timeStrs.value = [
      customDealKftime(props.beginTime),
      customDealKftime(props.endTime || props.nowTime),
    ];
  },
);
watch(
  () => props.beginTime,
  () => {
    // if (props.isTimeContinue) {
    //   timeStrs.value = [
    //     customDealKftime(props.beginTime),
    //     customDealKftime(props.endTime || props.nowTime),
    //   ];
    // } else {
    //   timeStrs.value = [
    //     customDealKftime(props.beginTime),
    //     customDealKftime(props.currentTime),
    //   ];
    // }
    timeStrs.value = [
      customDealKftime(props.beginTime),
      customDealKftime(props.endTime || props.nowTime),
    ];
  },
);
// watch(
//   () => props.isTimeContinue,
//   (newTimeContinue) => {
//     // console.log('newTimeContinue', newTimeContinue);
//     // if (newTimeContinue) {
//     //   timeStrs.value = [
//     //     customDealKftime(props.beginTime),
//     //     customDealKftime(props.endTime || props.nowTime),
//     //   ];
//     // } else {
//     //   timeStrs.value = [
//     //     customDealKftime(props.beginTime),
//     //     customDealKftime(props.currentTime),
//     //   ];
//     // }
//     timeStrs.value = [
//       customDealKftime(props.beginTime),
//       customDealKftime(props.currentTime),
//     ];
//   },
// );

watch(
  () => props.currentTime,
  () => {
    curTime.value = nano2millionSecond(props.currentTime);
  },
);

// const timeStrs = computed(() => {
//   return [
//     customDealKftime(props.currentTime[0]),
//     customDealKftime(props.currentTime[1]),
//   ];
// });

const tipFormatter = (num: number) => {
  return customDealKftime(BigInt(num * SCALE));
};

const dealUpdateTime = (time: number) => {
  // if (time === limitRangeResolved.value[0]) return props.limitTimeRange[0];
  // if (time === limitRangeResolved.value[1]) return props.limitTimeRange[1];
  return million2nanoSecond(time);
};

const onAfterChange = (value: number) => {
  if (
    // value[0] === nano2millionSecond(props.currentTime[0]) &&
    value === nano2millionSecond(props.nowTime)
  ) {
    // emit('timeContinueUpdate', true);
    // return;
  } else {
    // emit('timeContinueUpdate', false);
  }

  // if (props.stick) {
  //   // sticking.value[0] = value[0] === limitRangeResolved.value[0];
  //   sticking.value[1] = value === limitRangeResolved.value[1];
  // }
  console.log('onAfterChange', value);
  emit('update:currentTime', dealUpdateTime(value));
};

const resetInputData = (index: 0 | 1) => {
  timeInputData.value[index] = {
    inputting: false,
    value: '',
  };
};

const handleDbClickTimeText = (index: 0 | 1) => {
  timeInputData.value[index] = {
    inputting: true,
    value: timeStrs.value[index],
  };
};

const handleConfirmTimeInput = (index: 0 | 1) => {
  str2millionTime(timeInputData.value[index].value)
    .then((resolvedTime) => {
      curTime.value = resolvedTime;
      onAfterChange(curTime.value);
      resetInputData(index);
    })
    .catch((err) => {
      messagePrompt().error(err);
    });
};

// defineExpose({
//   sticking,
// });
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
    width: 160px;
    margin: 0 16px;
    flex: 0 0 160px;
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
    font-size: 26px; /* 改变图标大小 */
    color: #fff; /* 设置初始颜色 */
    transition: color 0.3s; /* 平滑过渡 */
  }

  .forward-icon:hover {
    color: #faad14; /* 鼠标悬停时的颜色 */
  }
}
</style>

<template>
  <div class="kf-time-slider__wrap">
    <div class="kf-time-slider-time">
      <a-input-group v-if="timeInputData[0].inputting" compact>
        <a-input
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
        </a-button>
      </a-input-group>
      <span
        v-else
        class="kf-time-slider-text"
        style="text-align: end"
        @dblclick="handleDbClickTimeText(0)"
      >
        {{ timeStrs[0] }}
      </span>
    </div>
    <a-slider
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
  </div>
</template>

<script lang="ts" setup>
import { computed, onMounted, onUnmounted, ref, watch } from 'vue';

import { CheckOutlined, CloseOutlined } from '@ant-design/icons-vue';
import { dealKfTime } from '@kungfu-trader/kungfu-js-api/kungfu';
import VueI18n from '@kungfu-trader/kungfu-js-api/language';
import { messagePrompt } from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/uiUtils';

const { t } = VueI18n.global;

type DoubleArray<T> = [T, T];

//  q:timeRage是从哪里来的？  从哪里传递过来的？
//  a: 从父组件传递过来的，父组件是Journal.vue
const props = withDefaults(
  defineProps<{
    timeRange: DoubleArray<bigint>; // 当前只支持纳秒级别的时间
    limitTimeRange: DoubleArray<bigint>; // 当前只支持纳秒级别的时间
    isExternalUpdate: boolean;
    nolRange: DoubleArray<bigint>;
    step: number;
    stick: boolean;
  }>(),
  {
    step: 10000000, // step 为纳秒级别， 默认为10毫秒
  },
);

const emit = defineEmits<{
  (e: 'update:timeRange', value: DoubleArray<bigint>): void;
  (e: 'externalUpdate', value: boolean): void;
}>();

const SCALE = 1000000;
const BIGINT_SCALE = BigInt(SCALE);

const isDragging = ref<boolean>(false);
const slider = ref();
const sticking = ref<DoubleArray<boolean>>([
  props.timeRange[0] === props.limitTimeRange[0],
  props.timeRange[0] === props.limitTimeRange[0],
]);
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
  timeStr = timeStr.replaceAll(' ', '');
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

const currentTimeRange = ref<DoubleArray<number>>([
  nano2millionSecond(props.timeRange[0]),
  nano2millionSecond(props.timeRange[1]),
]);

const limitRangeResolved = ref<DoubleArray<number>>([
  nano2millionSecond(props.limitTimeRange[0]),
  nano2millionSecond(props.limitTimeRange[1]),
]);

watch(
  () => props.nolRange,
  () => {
    if (props.isExternalUpdate) {
      console.log('llllll', {
        currentTimeRange: currentTimeRange.value,
        nolRange: props.nolRange,
      });
      onAfterChange([
        nano2millionSecond(props.nolRange[0]),
        nano2millionSecond(props.nolRange[1]),
      ]);
    }
  },
);
// watch(
//   () => currentTimeRange.value,
//   () => {
//     console.log('externalUpdate', props.isExternalUpdate);
//     emit('externalUpdate', false);
//   },
// );

watch(
  () => props.timeRange,
  (newRange) => {
    currentTimeRange.value = [
      nano2millionSecond(newRange[0]),
      nano2millionSecond(newRange[1]),
    ];
  },
);

watch(
  () => props.limitTimeRange,
  (newLimitRange) => {
    limitRangeResolved.value = [
      nano2millionSecond(newLimitRange[0]),
      nano2millionSecond(newLimitRange[1]),
    ];
    if (props.stick && !isDragging.value) {
      if (sticking.value[0]) {
        currentTimeRange.value[0] = limitRangeResolved.value[0];
      }

      if (sticking.value[1]) {
        currentTimeRange.value[1] = limitRangeResolved.value[1];
      }

      if (sticking.value.some((item) => item)) {
        emit('update:timeRange', [
          dealUpdateTime(currentTimeRange.value[0]),
          dealUpdateTime(currentTimeRange.value[1]),
        ]);
      }
    }
  },
);

const timeStrs = computed(() => {
  return [
    customDealKftime(props.timeRange[0]),
    customDealKftime(props.timeRange[1]),
  ];
});

const tipFormatter = (num: number) => {
  return customDealKftime(BigInt(num * SCALE));
};

const dealUpdateTime = (time: number) => {
  if (time === limitRangeResolved.value[0]) return props.limitTimeRange[0];
  if (time === limitRangeResolved.value[1]) return props.limitTimeRange[1];
  return million2nanoSecond(time);
};

const onAfterChange = (value: DoubleArray<number>) => {
  if (
    value[0] === nano2millionSecond(props.timeRange[0]) &&
    value[1] === nano2millionSecond(props.timeRange[1])
  )
    return;

  if (props.stick) {
    sticking.value[0] = value[0] === limitRangeResolved.value[0];
    sticking.value[1] = value[1] === limitRangeResolved.value[1];
  }
  emit('update:timeRange', [
    dealUpdateTime(value[0]),
    dealUpdateTime(value[1]),
  ]);
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
      currentTimeRange.value[index] = resolvedTime;
      onAfterChange(currentTimeRange.value);
      resetInputData(index);
    })
    .catch((err) => {
      messagePrompt().error(err);
    });
};

defineExpose({
  sticking,
});
</script>

<style lang="less">
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
}
</style>

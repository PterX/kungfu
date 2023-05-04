<template>
  <div class="kf-time-slider__wrap">
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
      <span v-else class="kf-time-slider-text" style="text-align: start">
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
import { onMounted, onUnmounted, ref, watch } from 'vue';

import { CheckOutlined, CloseOutlined } from '@ant-design/icons-vue';
import { dealKfTime } from '@kungfu-trader/kungfu-js-api/kungfu';
import VueI18n from '@kungfu-trader/kungfu-js-api/language';
import { messagePrompt } from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/uiUtils';
import { ForwardOutlined, BackwardOutlined } from '@ant-design/icons-vue';

const { t } = VueI18n.global;

type DoubleArray<T> = [T, T];

const props = withDefaults(
  defineProps<{
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
const SCALE = 1000000;
const BIGINT_SCALE = BigInt(SCALE);
const TEN_SECOND = BigInt(10000000000);
const isDragging = ref<boolean>(false);
const slider = ref();
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

const handleTimeBack = () => {
  curTime.value = nano2millionSecond(props.currentTime - TEN_SECOND);
  onAfterChange(curTime.value);
};
const handleTimeForward = () => {
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
  return customDealKftime(BigInt(num * SCALE));
};

const dealUpdateTime = (time: number) => {
  return million2nanoSecond(time);
};

const onAfterChange = (value: number) => {
  emit('update:currentTime', dealUpdateTime(value));
};

const resetInputData = (index: 0 | 1) => {
  timeInputData.value[index] = {
    inputting: false,
    value: '',
  };
};

// const handleDbClickTimeText = (index: 0 | 1) => {
//   timeInputData.value[index] = {
//     inputting: true,
//     value: timeStrs.value[index],
//   };
// };

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
    font-size: 26px;
    color: #fff;
    transition: color 0.3s;
  }

  .forward-icon:hover {
    color: #faad14;
  }
}
</style>

<script lang="ts" setup>
import { onMounted, ref } from 'vue';
import { ReloadOutlined } from '@ant-design/icons-vue';
import { getNanoDateString } from '@kungfu-trader/kungfu-js-api/kungfu';

import VueI18n from '@kungfu-trader/kungfu-js-api/language';

const { t } = VueI18n.global;
const formRef = ref();
const props = withDefaults(
  defineProps<{
    isJournal?: boolean;
    visible: boolean;
    sessionOptions: {
      label: string;
      value: string;
    }[];
    sessionInfo: string;
    beginTime: string;
    endTime: string;
    logLevel: string;
  }>(),
  {
    isJournal: false,
    visible: false,
    sessionOptions: () => [],
  },
);

const emit = defineEmits<{
  (
    e: 'confirm',
    data: {
      sessionInfo: string;
      beginTime: string;
      endTime: string;
      logLevel: string;
    },
  ): void;
  (e: 'update:visible', visible: boolean): void;
  (e: 'close'): void;
}>();

const dealEndTime = () => {
  let endTime = '';
  for (let i of props.sessionOptions) {
    if (i.value.split('--')[0] === formState.value.beginTime) {
      endTime = i.value.split('--')[1];
    }
  }
  if (endTime === 'now') {
    endTime = getNanoDateString(BigInt(new Date().getTime()) * 1000000n);
  }
  const a = formatTimeToNanoseconds(formState.value.endTime, [
    props.beginTime,
    endTime,
  ]);
  formState.value.endTime = a;
};
onMounted(() => {
  dealEndTime();
  formState.value.sessionInfo = props.sessionInfo;
});

const logLevelOptions = [
  { value: '-l trace', label: 'TRACE' },
  { value: '-l debug', label: 'DEBUG' },
  { value: '-l info', label: 'INFO' },
  { value: '-l warning', label: 'WARN' },
  { value: '-l error', label: 'ERROR' },
  { value: '-l critical', label: 'CRITICAL' },
];

const formState = ref({
  sessionInfo: '',
  beginTime: props.beginTime || '',
  endTime: props.endTime || '',
  logLevel: props.logLevel || '-l info',
});
const handleSelectSessionInfo = (value: string) => {
  formState.value.beginTime = value.split('--')[0];
  formState.value.endTime =
    value.split('--')[1] !== 'now'
      ? value.split('--')[1]
      : getNanoDateString(BigInt(new Date().getTime()) * 1000000n);
};

const formatTimeToNanoseconds = (
  timeStr: string,
  boundaries: [string, string],
): string => {
  const format = (str: string) => {
    const regex = /^([0-9]{1,2}):([0-9]{1,2}):([0-9]{1,2})(?:\.([0-9]{1,9}))?$/;
    const match = str.match(regex);
    if (!match) {
      return boundaries[1];
    }
    const [, hoursStr, minutesStr, secondsStr, nanosecondsStr] = match;
    const hours = hoursStr.padStart(2, '0');
    const minutes = minutesStr.padStart(2, '0');
    const seconds = secondsStr.padStart(2, '0');
    const nanoseconds = (nanosecondsStr || '').padEnd(9, '0');
    return `${hours}:${minutes}:${seconds}.${nanoseconds}`;
  };

  const formattedTime = format(timeStr);
  const [lowerBound, upperBound] = boundaries.map(format);

  if (!formattedTime || !lowerBound || !upperBound) {
    return getNanoDateString(BigInt(new Date().getTime()) * 1000000n);
  }

  if (formattedTime < lowerBound || formattedTime > upperBound) {
    return upperBound;
  }

  return formattedTime;
};

const refreshEndTime = () => {
  let endTime = '';
  for (let i of props.sessionOptions) {
    if (i.value.split('--')[0] === formState.value.beginTime) {
      endTime = i.value.split('--')[1];
    }
  }

  if (endTime === 'now') {
    endTime = getNanoDateString(BigInt(new Date().getTime()) * 1000000n);
  }

  formState.value.endTime = endTime;
};

const handleOk = () => {
  formRef.value
    ?.validate()
    .then(() => {
      emit('confirm', formState.value);
    })
    .catch((error) => {
      console.error('Validation failed:', error);
    });
};

const handleCancel = () => {
  emit('close');
};
</script>

<style lang="less">
.replay-config-form {
  .ant-form-item-label {
    flex: 0 0 25%;
    max-width: 25%;
  }
  .ant-form-item-control {
    max-width: 58.33333333%;
  }
}
</style>

<template>
  <a-modal
    :visible="visible"
    :maskClosable="false"
    class="kf-set-by-config-modal"
    :title="$t('replay.replay')"
    :ok-text="$t('confirm')"
    :cancel-text="$t('cancel')"
    @ok="handleOk"
    @cancel="handleCancel"
  >
    <div class="flex-container">
      <a-form
        :model="formState"
        name="basic"
        autocomplete="off"
        ref="formRef"
        class="replay-config-form"
      >
        <a-form-item :label="$t('replay.session')" name="sessionInfo">
          <a-select
            v-model:value="formState.sessionInfo"
            @select="handleSelectSessionInfo"
          >
            <a-select-option
              v-for="item in sessionOptions"
              :key="item.value"
              :value="item.value"
            >
              {{ item.label }}
            </a-select-option>
          </a-select>
        </a-form-item>
        <a-form-item
          :label="$t('replay.begin_time')"
          name="beginTime"
          :required:="true"
        >
          <a-input v-model:value="formState.beginTime" :disabled="true" />
        </a-form-item>
        <a-form-item :label="$t('replay.end_time')" name="endTime">
          <a-input v-model:value="formState.endTime" @blur="dealEndTime">
            <template v-slot:suffix>
              <ReloadOutlined @click="refreshEndTime" style="font-size: 14px" />
            </template>
          </a-input>
        </a-form-item>
        <a-form-item
          :label="$t('replay.log_level')"
          name="logLevel"
          :rules="[
            {
              required: true,
              message: t('replay.please_select_log_level'),
              trigger: 'blur',
            },
          ]"
        >
          <a-select v-model:value="formState.logLevel">
            <a-select-option
              v-for="item in logLevelOptions"
              :key="item.value"
              :value="item.value"
            >
              {{ item.label }}
            </a-select-option>
          </a-select>
        </a-form-item>
      </a-form>
      <div class="form-note">
        <div class="spacer"></div>
        <div class="note-content" v-if="props.isJournal">
          <p>* 注:</p>
          <ul>
            <li>
              1.回放依赖数据时间,
              <span class="highlighted-text">
                请勿在策略内使用物理时间相关代码来表达"now"
              </span>
              (如 python 的 time 或者 yjj.now_in_nano), 建议使用策略提供的
              context.now 方法, 否则回放无法按照预期执行.
            </li>
            <li>
              2.回放仅可增加 log,
              <span class="highlighted-text">请勿修改策略逻辑</span>
              , 否则会由于输出数据与实际不符而报错.
            </li>
          </ul>
        </div>
        <div class="note-content" v-else>
          <p>* 注:</p>
          <ul>
            <li>
              1.点击确认后, 开始回放本记录最近一次 session.如需回放之前启动过的
              session, 请使用 journal 工具选择 session, 点击"回放"按钮进行回放.
            </li>
            <li>
              2.回放依赖数据时间,
              <span class="highlighted-text">
                请勿在策略内使用物理时间相关代码来表达"now"
              </span>
              (如 python 的 time 或者 yjj.now_in_nano), 建议使用策略提供的
              context.now 方法, 否则回放无法按照预期执行.
            </li>
            <li>
              3.回放仅可增加 log,
              <span class="highlighted-text">请勿修改策略逻辑</span>
              , 否则会由于输出数据与实际不符而报错.
            </li>
          </ul>
        </div>
      </div>
    </div>
  </a-modal>
</template>

<style lang="less">
.flex-container {
  display: flex;
  flex-direction: column;
}

.highlighted-text {
  color: #faad14;
}

.form-note {
  display: flex;
  font-size: 12px;
}

.spacer {
  flex: 0 0 12%;
}

.note-content {
  flex: 1;
  ul {
    padding-left: 20px;
    margin: 0;

    li {
      margin-bottom: 10px;
    }
  }
}
.note-content ul {
  list-style-type: none;
  padding-left: 0;
}
</style>

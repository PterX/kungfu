<script lang="ts" setup>
import { onMounted, ref } from 'vue';
import { useGlobalStore } from '@kungfu-trader/kungfu-app/src/renderer/pages/index/store/global';
import VueI18n from '@kungfu-trader/kungfu-js-api/language';

const { t } = VueI18n.global;
const formRef = ref();
const props = withDefaults(
  defineProps<{
    visible: boolean;
    sessionOptions: {
      label: string;
      value: string;
    }[];
    beginTime: string;
    endTime: string;
    logLevel: string;
  }>(),
  {
    visible: false,
    sessionOptions: () => [],
  },
);
console.log(props);
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

onMounted(() => {
  formState.value.endTime = formatTimeToNanoseconds(formState.value.endTime, [
    props.beginTime,
    props.endTime,
  ]);
});

const logLevelOptions = [
  { value: '-l trace', label: 'TRACE' },

  { value: '-l debug', label: 'DEBUG' },
  { value: '-l info', label: 'INFO' },
  { value: '-l warn', label: 'WARN' },
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
  if (value !== 'now')
    formState.value.endTime =
      value.split('--')[1] !== 'now' ? value.split('--')[1] : props.endTime;
};

const formatTimeToNanoseconds = (
  timeStr: string,
  boundaries: [string, string],
): string => {
  const format = (str: string) => {
    const regex = /^([0-9]{1,2}):([0-9]{1,2}):([0-9]{1,2})(?:\.([0-9]{1,9}))?$/;
    const match = str.match(regex);
    if (!match) {
      return null;
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
    return 'Invalid time format';
  }

  if (formattedTime < lowerBound || formattedTime > upperBound) {
    return upperBound;
  }

  return formattedTime;
};

const dealEndTime = () => {
  const a = formatTimeToNanoseconds(formState.value.endTime, [
    props.beginTime,
    props.endTime,
  ]);
  formState.value.endTime = a;
  console.log('endTime', a);
};

const handleOk = () => {
  formRef.value
    ?.validate()
    .then(() => {
      console.log(formState.value);
      useGlobalStore().setReplaySetting({
        begin_time: formState.value.beginTime,
        end_time: formState.value.endTime,
        log_level: formState.value.logLevel,
      });
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
        <a-input v-model:value="formState.endTime" @blur="dealEndTime" />
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
  </a-modal>
</template>

<script lang="ts" setup>
import { onMounted, ref } from 'vue';
import { ReloadOutlined } from '@ant-design/icons-vue';
import { getKfExtOriginConfigsByType } from '@kungfu-trader/kungfu-js-api/utils/extUtils';

import VueI18n from '@kungfu-trader/kungfu-js-api/language';
import { useReplay } from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/actionsUtils';

const { t } = VueI18n.global;
const formRef = ref();
const props = withDefaults(
  defineProps<{
    isJournal?: boolean;
    canBacktest?: boolean;
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

const { formatSessionTime } = useReplay();

const SessionTimeDivider = '--';

const getSessionTimeRange = (timeStr: string) => {
  const [sessionBeginTime, sessionEndTime] = timeStr.split(SessionTimeDivider);
  return { sessionBeginTime, sessionEndTime };
};

const dealEndTime = () => {
  let endTime = '';
  for (let i of props.sessionOptions) {
    const { sessionBeginTime, sessionEndTime } = getSessionTimeRange(i.value);
    if (sessionBeginTime === formState.value.beginTime) {
      endTime = sessionEndTime;
    }
  }
  if (endTime === 'now') {
    endTime = formatSessionTime(BigInt(new Date().getTime()) * 1000000n);
  }
  formState.value.endTime = formatTimeToNanoseconds(formState.value.endTime, [
    props.beginTime,
    endTime,
  ]);
};

const getBacktestConfig = async () => {
  const extOriginConfigs = await getKfExtOriginConfigsByType();
  let hasIndexer = false;
  let hasMatcher = false;
  if (extOriginConfigs) {
    const { matcher, indexer } = extOriginConfigs;

    if (indexer) {
      for (const key of Object.keys(indexer)) {
        if (indexer[key]['useFor']?.includes('replay')) {
          hasIndexer = true;
          break;
        }
      }
    }

    if (matcher && Object.keys(matcher).length > 0) {
      hasMatcher = true;
    }
  }

  isShowMatcher.value = hasMatcher && hasIndexer;
};
onMounted(() => {
  props.canBacktest ? getBacktestConfig() : '';
  dealEndTime();
  if (props.sessionInfo) {
    const { sessionEndTime } = getSessionTimeRange(props.sessionInfo);
    if (sessionEndTime === 'now') {
      formState.value.endTime = formatSessionTime(
        BigInt(new Date().getTime()) * 1000000n,
      );
    }
  }
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

const isShowMatcher = ref(false);

const formState = ref({
  sessionInfo: '',
  beginTime: props.beginTime || '',
  endTime: props.endTime || '',
  logLevel: props.logLevel || '-l info',
  enableMatcher: false,
});
const handleSelectSessionInfo = (value: string) => {
  const { sessionBeginTime, sessionEndTime } = getSessionTimeRange(value);
  formState.value.beginTime = sessionBeginTime;
  formState.value.endTime =
    sessionEndTime !== 'now'
      ? sessionEndTime
      : formatSessionTime(BigInt(new Date().getTime()) * 1000000n);
};

const formatTimeToNanoseconds = (
  timeStr: string,
  boundaries: [string, string],
): string => {
  const format = (str: string) => {
    const regex =
      /^(\d{4}-\d{2}-\d{2}) (\d{2}):(\d{2}):(\d{2})(?:\.(\d{1,9}))?$/;
    const match = str.match(regex);
    if (!match) {
      return boundaries[1];
    }
    const [, dateStr, hoursStr, minutesStr, secondsStr, nanosecondsStr] = match;
    const hours = hoursStr.padStart(2, '0');
    const minutes = minutesStr.padStart(2, '0');
    const seconds = secondsStr.padStart(2, '0');
    const nanoseconds = (nanosecondsStr || '').padEnd(9, '0');
    return `${dateStr} ${hours}:${minutes}:${seconds}.${nanoseconds}`;
  };

  const formattedTime = format(timeStr);
  const [lowerBound, upperBound] = boundaries.map(format);

  if (!formattedTime || !lowerBound || !upperBound) {
    return formatSessionTime(BigInt(new Date().getTime()) * 1000000n);
  }

  if (formattedTime < lowerBound || formattedTime > upperBound) {
    return upperBound;
  }

  return formattedTime;
};

const refreshEndTime = () => {
  let endTime = '';
  for (let i of props.sessionOptions) {
    const { sessionBeginTime, sessionEndTime } = getSessionTimeRange(i.value);
    if (sessionBeginTime === formState.value.beginTime) {
      endTime = sessionEndTime;
    }
  }

  if (endTime === 'now') {
    endTime = formatSessionTime(BigInt(new Date().getTime()) * 1000000n);
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
      console.error(error);
    });
};

const handleCancel = () => {
  emit('close');
};
</script>

<template>
  <a-modal
    :visible="visible"
    :mask-closable="false"
    class="kf-set-by-config-modal"
    :title="$t('replay.replay')"
    :ok-text="$t('confirm')"
    :cancel-text="$t('cancel')"
    @ok="handleOk"
    @cancel="handleCancel"
  >
    <div class="flex-container">
      <a-form
        ref="formRef"
        :model="formState"
        name="basic"
        autocomplete="off"
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
            <template #suffix>
              <ReloadOutlined style="font-size: 14px" @click="refreshEndTime" />
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
        <a-form-item
          v-if="isShowMatcher"
          :label="$t('replay.enable_matcher')"
          name="enableMatcher"
          :extra="$t('replay.enable_matcher_tip')"
        >
          <a-switch
            v-model:checked="formState.enableMatcher"
            size="small"
          ></a-switch>
        </a-form-item>
      </a-form>
      <div class="form-note">
        <div class="spacer"></div>
        <div v-if="props.isJournal" class="note-content">
          <p>{{ t('replay.tips_title') }}</p>
          <ul>
            <li>
              {{ `1. ${t('replay.replay_tips1_part1')}` }}
              <span class="highlighted-text">
                {{ t('replay.replay_tips1_part2') }}
              </span>
              {{ t('replay.replay_tips1_part3') }}
            </li>
            <li>
              {{ `2. ` }}
              <span class="highlighted-text">
                {{ `${t('replay.replay_tips2_part1')}` }}
              </span>
              {{ t('replay.replay_tips2_part2') }}
              <span class="highlighted-text">
                {{ t('replay.replay_tips2_part3') }}
              </span>
              {{ t('replay.replay_tips2_part4') }}
              <span class="highlighted-text">
                {{ t('replay.replay_tips2_part5') }}
              </span>
              {{ t('replay.replay_tips2_part6') }}
              <span class="highlighted-text">
                {{ t('replay.replay_tips2_part7') }}
              </span>
              {{ t('replay.replay_tips2_part8') }}
            </li>
          </ul>
        </div>
        <div v-else class="note-content">
          <p>{{ t('replay.tips_title') }}</p>
          <ul>
            <li>
              {{ `1. ${t('replay.replay_tips1_part1')}` }}
              <span class="highlighted-text">
                {{ t('replay.replay_tips1_part2') }}
              </span>
              {{ t('replay.replay_tips1_part3') }}
            </li>
            <li>
              {{ `2. ` }}
              <span class="highlighted-text">
                {{ `${t('replay.replay_tips2_part1')}` }}
              </span>
              {{ t('replay.replay_tips2_part2') }}
              <span class="highlighted-text">
                {{ t('replay.replay_tips2_part3') }}
              </span>
              {{ t('replay.replay_tips2_part4') }}
              <span class="highlighted-text">
                {{ t('replay.replay_tips2_part5') }}
              </span>
              {{ t('replay.replay_tips2_part6') }}
              <span class="highlighted-text">
                {{ t('replay.replay_tips2_part7') }}
              </span>
              {{ t('replay.replay_tips2_part8') }}
            </li>
          </ul>
        </div>
      </div>
    </div>
  </a-modal>
</template>

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
      margin-bottom: 4px;
    }
  }
}
.note-content ul {
  list-style-type: none;
  padding-left: 0;
}
</style>

<template>
  <LogView ref="logViewRef" :log-path="logPath">
    <template #title>
      <KfDashboardItem>
        <div class="replay_title">
          {{ enableMatcher ? $t('replay.backtest') : $t('replay.replay') }}
        </div>
      </KfDashboardItem>
      <KfDashboardItem>
        <div class="replay_title">
          {{ `${$t('replay.log_level')}: ${replayLogLevel}` }}
        </div>
      </KfDashboardItem>
      <KfDashboardItem>
        <div class="replay_title">
          {{ `${$t('replay.begin_time')}: ${timeRange.beginTime}` }}
        </div>
      </KfDashboardItem>
      <KfDashboardItem>
        <div class="replay_title">
          {{ `${$t('replay.end_time')}: ${timeRange.endTime}` }}
        </div>
      </KfDashboardItem>
    </template>
    <template #action>
      <KfDashboardItem>
        <a-button
          @click="reLoadLog"
          size="small"
          :loading="isLoading || preLoading"
        >
          {{ $t('replay.try_again') }}
        </a-button>
      </KfDashboardItem>
    </template>
  </LogView>
</template>

<script setup lang="ts">
import KfDashboardItem from '@kungfu-trader/kungfu-app/src/renderer/components/public/KfDashboardItem.vue';

import LogView from '@kungfu-trader/kungfu-app/src/renderer/pages/logview/LogView.vue';
import { ipcRenderer } from 'electron';
import { getCurrentWindow } from '@electron/remote';
import { ipcEmit } from '@kungfu-trader/kungfu-app/src/renderer/ipcMsg/emitter';
import { ensureFileSync, outputFile } from 'fs-extra';
import { useRemoveReplayProcess } from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/actionsUtils';
import { messagePrompt } from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/uiUtils';
import { LogLevelType } from '@kungfu-trader/kungfu-app/src/typings/enums';

import { listProcessStatus } from '@kungfu-trader/kungfu-js-api/utils/processUtils';
import { computed, onBeforeUnmount, onMounted, ref, watch } from 'vue';
import VueI18n from '@kungfu-trader/kungfu-js-api/language';

const { t } = VueI18n.global;

defineExpose({
  updateLogLevel,
});
const { error } = messagePrompt();
const { handleRemoveReplayProcess } = useRemoveReplayProcess();

const logViewRef = ref();
const currentWindow = getCurrentWindow();

const props = withDefaults(
  defineProps<{
    params: Partial<{
      category: string;
      group: string;
      beginTime: string;
      endTime: string;
      logPath: string;
      logLevel: string;
      sessionName: string;
      filePath: string;
      processId: string;
      enableMatcher: string;
      replayPreLoading: boolean;
    }>;
    closeImmediately?: boolean;
  }>(),
  {
    closeImmediately: true,
  },
);

const emit = defineEmits<{
  (e: 'stopReplayLoading'): void;
}>();

const RELOADING_TIMER = 10000;
let reloadingTimer: NodeJS.Timeout | null = null;

const startReloading = ref(false);

const preLoading = computed(() => {
  return startReloading.value || props.params.replayPreLoading;
});

const enableMatcher = computed(() => {
  return props.params.enableMatcher === 'true';
});

const replayLogLevel = computed(
  () => LogLevelType[props.params.logLevel ? props.params.logLevel : ''] || '',
);

const timeRange = computed(() => ({
  beginTime: props.params.beginTime || '',
  endTime: props.params.endTime || '',
}));

const logPath = computed(() => {
  return props.params.logPath || '';
});

const CHECK_REPLAY_PROCESS_TIMER = 1000;
const isLoading = ref(false);
onMounted(async () => {
  ipcRenderer.on('clear-process', async (_event, args) => {
    const { processId } = args;
    if (processId === props.params.processId) {
      logViewRef.value && logViewRef.value.resetLog();
    }
  });
  const replayPocessCheckTimer = setInterval(async () => {
    const { processStatus } = await listProcessStatus();
    if (processStatus) {
      if (
        props.params.processId &&
        processStatus[props.params.processId] === 'online'
      ) {
        emit('stopReplayLoading');
        startReloading.value = false;
        isLoading.value = true;
        if (reloadingTimer) {
          clearTimeout(reloadingTimer);
          reloadingTimer = null;
        }
      } else {
        isLoading.value = false;
      }
    }
  }, CHECK_REPLAY_PROCESS_TIMER);
  if (!currentWindow) {
    return;
  }
  const pawin = currentWindow.getParentWindow();
  if (!pawin) return;

  if (!props.closeImmediately) {
    currentWindow.on('close', async (event) => {
      event.preventDefault();
      const { processStatus } = await listProcessStatus();
      if (
        props.params.processId &&
        processStatus[props.params.processId] !== 'online'
      ) {
        currentWindow.destroy();
      }
      props.params.processId &&
        handleRemoveReplayProcess(props.params.processId).finally(() => {
          currentWindow.destroy();
        });
    });
  }
  onBeforeUnmount(() => {
    clearInterval(replayPocessCheckTimer);
  });
});

watch(
  () => props.params.logLevel,
  (newVal) => {
    replayLogLevel.value =
      LogLevelType[newVal ? newVal.replace('%20', ' ') : ''] || '';
  },
);

const throwError = (messageKey: string) => {
  error(t(messageKey));
};

async function reLoadLog() {
  if (reloadingTimer) {
    clearTimeout(reloadingTimer);
    reloadingTimer = null;
  }

  if (!currentWindow) {
    return;
  }

  const { processStatus } = await listProcessStatus();
  const processId = props.params.processId || '';

  if (!processStatus[processId]) {
    throwError('replay.please_start_replay');
    return;
  }

  const pawin = currentWindow.getParentWindow();
  if (!pawin) {
    return;
  }

  const configs = localStorage.getItem('replayConfigs');
  if (!configs) {
    throwError('replay.please_start_replay');
    return;
  }

  const replayArgs = JSON.parse(configs);
  const config = replayArgs?.[processId];
  if (!config) {
    throwError('replay.please_start_replay');
    return;
  }

  const { args: configArgs, filePath: currentFile } = config;
  const {
    beginTime,
    endTime,
    filePath: paramsFilePath,
    logLevel,
    sessionName,
  } = props.params;
  const rerunFlag =
    configArgs?.replayConfig?.begin_time === beginTime &&
    configArgs?.replayConfig?.end_time === endTime;

  const filePath = paramsFilePath || currentFile;
  const replayConfig = {
    category: configArgs.category,
    group: configArgs.group,
    begin_time: beginTime,
    end_time: endTime,
    log_level: logLevel ? logLevel.replace('%20', ' ') : '-l info',
    session_name: sessionName,
    file_path: filePath,
    enable_matcher: enableMatcher.value,
  };

  const args = rerunFlag
    ? configArgs
    : {
        ...configArgs,
        replayConfig,
      };

  if (!rerunFlag) {
    replayArgs[processId].args = args;
    localStorage.setItem('replayConfigs', JSON.stringify(replayArgs));
    replayLogLevel.value = LogLevelType[args.replayConfig.log_level];
  }

  try {
    ensureFileSync(logPath.value);
    await outputFile(logPath.value, '');
    if (props.params.replayPreLoading) {
      emit('stopReplayLoading');
    }
    startReloading.value = true;
    reloadingTimer = setTimeout(() => {
      startReloading.value = false;
    }, RELOADING_TIMER);
    await ipcEmit('clear-process', { processId: processId || '' });
    logViewRef.value?.resetLog();
    pawin.webContents.send('startReplay', {
      replayProcessParams: args,
    });
  } catch (err) {
    console.error(err);
  }
}

function updateLogLevel() {
  const configs = localStorage.getItem('replayConfigs');
  if (configs) {
    const config = (JSON.parse(configs) || {})[props.params.processId || ''];
    if (config) {
      const replayParams = config.args;
      if (replayParams && replayParams.replayConfig) {
        replayLogLevel.value =
          LogLevelType[replayParams.replayConfig.log_level];
      }
    }
  }
}
</script>

<style lang="less">
@import '@kungfu-trader/kungfu-app/src/renderer/assets/less/base.less';
@import '@kungfu-trader/kungfu-app/src/renderer/assets/less/public.less';
@import '@kungfu-trader/kungfu-app/src/renderer/assets/less/coverAnt.less';
@import '@kungfu-trader/kungfu-app/src/renderer/assets/less/variables.less';

.default-log-view_warp {
  height: 100%;
  .ant-layout {
    height: 100%;
    background: @component-background;

    .kf-dashboard__header {
      .kf-dashboard-item__warp {
        .search-in-table__warp {
          display: flex;
          justify-content: flex-start;
          align-items: center;
          font-size: 12px;
          margin-right: 4px;

          .search-int-table__item {
            margin: 0 4px;
          }
        }
      }
    }

    .kf-dashboard__body {
      background: #000;
      padding: 8px;
      box-sizing: border-box;
      border-radius: 4px;
    }
  }

  .kf-log-view__warp {
    height: 100%;
    width: 100%;
    padding: 0 8px 8px 8px;
  }

  .replay_title {
    font-size: 12px;
  }

  .kf-log-line {
    text-align: left;
    font-size: 14px;
    user-select: text;
    padding-bottom: 4px;
    line-height: 1.5;
    word-break: break-all;

    .error {
      color: lighten(@red2-base, 10%);
      font-weight: bold;
    }

    .debug {
      color: @blue-6;
      font-weight: bold;
    }

    .info {
      color: @green2-base;
      font-weight: bold;
    }

    .warning {
      color: @orange-6;
      font-weight: bold;
    }

    .trace {
      color: @cyan-6;
      font-weight: bold;
    }

    .critical {
      color: lighten(@red2-base, 10%);
      font-weight: bold;
    }

    .search-keyword {
      background: fade(@white, 70%);
      color: #000;
      font-weight: normal;

      &.current-search-pointer {
        background: @primary-color;
        color: #fff;
      }
    }
  }
}
</style>

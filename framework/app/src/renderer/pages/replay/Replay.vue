<template>
  <LogView ref="logViewRef" :log-path="LOG_PATH">
    <template #title>
      <KfDashboardItem>
        <div class="replay_title">
          {{ $t('replay.replay') }}
        </div>
      </KfDashboardItem>
      <KfDashboardItem>
        <div class="replay_title">
          {{ `${$t('replay.log_level')}: ${logLevel}` }}
        </div>
      </KfDashboardItem>
      <KfDashboardItem>
        <div class="replay_title">
          {{ `${$t('replay.begin_time')}: ${props.params.beginTime}` }}
        </div>
      </KfDashboardItem>
      <KfDashboardItem>
        <div class="replay_title">
          {{ `${$t('replay.end_time')}: ${props.params.endTime}` }}
        </div>
      </KfDashboardItem>
    </template>
    <template #action>
      <KfDashboardItem>
        <a-button @click="reLoadLog" size="small" :loading="isLoading">
          {{ $t('replay.try_again') }}
        </a-button>
      </KfDashboardItem>
    </template>
  </LogView>
</template>

<script setup lang="ts">
import KfDashboardItem from '@kungfu-trader/kungfu-app/src/renderer/components/public/KfDashboardItem.vue';

import LogView from '@kungfu-trader/kungfu-app/src/renderer/pages/logview/LogView.vue';
import { BrowserWindow } from '@electron/remote';
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
const currentWindow = BrowserWindow.getFocusedWindow();

const props = withDefaults(
  defineProps<{
    params: Record<string, string>;
    closeImmediately?: boolean;
  }>(),
  {
    closeImmediately: true,
  },
);

const logLevel = ref(
  LogLevelType[
    props.params.logLevel ? props.params.logLevel.replace('%20', ' ') : ''
  ] || '',
);

const LOG_PATH = props.params.logPath || '';
const CHECK_REPLAY_PROCESS_TIMER = 1000;
const isLoading = ref(false);
const replayList = ['strategy', 'operator'];
const isReplayAble = computed(() => {
  return replayList.includes(props.params.category);
});

onMounted(() => {
  const replayPocessCheckTimer = setInterval(async () => {
    const { processStatus } = await listProcessStatus();
    if (processStatus) {
      if (processStatus[props.params.processId] === 'online') {
        isLoading.value = true;
      } else {
        isLoading.value = false;
      }
    }
  }, CHECK_REPLAY_PROCESS_TIMER);
  if (!props.closeImmediately) {
    if (currentWindow) {
      if (!currentWindow.getParentWindow()) {
        currentWindow.destroy();
      }
      currentWindow.on('close', async (event) => {
        event.preventDefault();
        handleRemoveReplayProcess(props.params.processId).finally(() => {
          currentWindow.destroy();
        });
      });
    }
  }
  onBeforeUnmount(() => {
    clearInterval(replayPocessCheckTimer);
  });
});

watch(
  () => props.params.logLevel,
  (newVal) => {
    logLevel.value =
      LogLevelType[newVal ? newVal.replace('%20', ' ') : ''] || '';
  },
);
async function reLoadLog() {
  if (!isReplayAble.value) {
    messagePrompt().error(
      t('replay.only_operator_or_strategy_can_be_replayed'),
    );
    return;
  }

  if (currentWindow) {
    const pawin = currentWindow.getParentWindow();
    if (pawin) {
      const config = localStorage.getItem(props.params.processId);
      if (!config) {
        error(t('replay.please_start_replay'));
        return;
      }
      ensureFileSync(LOG_PATH);
      outputFile(LOG_PATH, '')
        .then(() => {
          logViewRef.value.resetLog();
          pawin.webContents.send('startReplay', {
            replayProcessParams: JSON.parse(config),
          });
        })
        .catch((err: Error) => {
          error(err.message || t('operation_failed'));
        });
    }
  }
}

function updateLogLevel(level: string) {
  const config = localStorage.getItem(props.params.processId);
  if (config) {
    const replayParams = JSON.parse(config);
    if (replayParams && replayParams.replayConfig) {
      logLevel.value = LogLevelType[replayParams.replayConfig.log_level];
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

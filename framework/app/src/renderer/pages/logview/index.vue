<script setup lang="ts">
import { onMounted, ref, computed, onBeforeUnmount } from 'vue';
// import { storeToRefs } from 'pinia';
import {
  UpOutlined,
  DownOutlined,
  ReloadOutlined,
} from '@ant-design/icons-vue';

import {
  messagePrompt,
  removeLoadingMask,
  setHtmlTitle,
useScrollerTableSearch,
} from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/uiUtils';
import { useRemoveReplayProcess } from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/actionsUtils';
// import { useJournalStore } from '@kungfu-trader/kungfu-app/src/renderer/pages/journal/store/journalStore';
import KfDashboard from '@kungfu-trader/kungfu-app/src/renderer/components/public/KfDashboard.vue';
import KfDashboardItem from '@kungfu-trader/kungfu-app/src/renderer/components/public/KfDashboardItem.vue';
import { LogLevelType } from '@kungfu-trader/kungfu-app/src/typings/enums';
import { ensureFileSync, outputFile } from 'fs-extra';
import { shell, BrowserWindow } from '@electron/remote';
import {
  useLogInit,
  dealLogMessage
} from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/logUtils';
import VueI18n from '@kungfu-trader/kungfu-js-api/language';
import { listProcessStatus } from '@kungfu-trader/kungfu-js-api/utils/processUtils';

// const { replayProcessConfigMap } = storeToRefs(useJournalStore());

const { handleRemoveReplayProcess } = useRemoveReplayProcess();

const currentWindow = BrowserWindow.getFocusedWindow();

const { t } = VueI18n.global;
const { success, error } = messagePrompt();

const props = withDefaults(
  defineProps<{
    params: Record<string, string>;
    type?: string;
    isJournal?: boolean;
    replayProcessParams?: {
      category: string;
      group: string;
      replayConfig: KungfuApi.ReplayConfig;
    };
  }>(),
  {
    isJournal: false,
  },
);

const CHECK_REPLAY_PROCESS_TIMER = 1000;
const params = props.params;
const isLoading = ref(false);
const LOG_PATH = params.logPath || '';
const replayList = ['strategy', 'operator'];
const isReplayAble = computed(() => {
  return replayList.includes(props.params.category);
});

setHtmlTitle(LOG_PATH);

const boardSize = ref<{ width: number; height: number }>({
  width: 0,
  height: 0,
});

const handleChangeBoardSize = ({
  width,
  height,
}: {
  width: number;
  height: number;
}) => {
  boardSize.value.width = width;
  boardSize.value.height = height;
};

const {
  logList,
  scrollToBottomChecked,
  scrollerTableRef,
  scrollToBottom,
  startTailLog,
  clearLogState,
} = useLogInit(LOG_PATH);

const {
  inputSearchRef,
  searchKeyword,
  currentResultIndex,
  totalResultCount,
  clearSearchState,
  handleToDownSearchResult,
  handleToUpSearchResult,
  getItemHtmlResult,
} = useScrollerTableSearch(
  () => logList.list,
  'id',
  ['message'],
  scrollerTableRef,
);

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
  if (props.type && props.type === 'replay' && !props.isJournal) {
    if (currentWindow) {
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

  removeLoadingMask();
  resetLog();
});


function handleRemoveLog(): Promise<void> {
  ensureFileSync(LOG_PATH);
  return outputFile(LOG_PATH, '')
    .then(() => {
      success();
      resetLog();
    })
    .catch((err: Error) => {
      error(err.message || t('operation_failed'));
    });
}

function handleOpenFileLocation() {
  return shell.showItemInFolder(LOG_PATH);
}

function resetLog() {
  clearLogState();
  clearSearchState()
  startTailLog();
}
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
      if (props.isJournal) {
        const config = localStorage.getItem(props.params.processId);
        if (!config) {
          messagePrompt().error(t('replay.please_start_replay'));
          return;
        }
        ensureFileSync(LOG_PATH);
        outputFile(LOG_PATH, '')
          .then(() => {
            resetLog();
            pawin.webContents.send('startReplay', {
              replayProcessParams: JSON.parse(config),
            });
          })
          .catch((err: Error) => {
            error(err.message || t('operation_failed'));
          });

        return;
      } else if (!props.isJournal) {
        ensureFileSync(LOG_PATH);
        outputFile(LOG_PATH, '')
          .then(() => {
            resetLog();
            pawin.webContents.send('trigger-main-window-hook', {
              hookName: props.params.processId,
              key: 'start',
            });
          })
          .catch((err: Error) => {
            error(err.message || t('operation_failed'));
          });
      } else {
        messagePrompt().error(t('replay.please_start_replay'));
      }
    }
  }
}
</script>
<template>
  <div class="default-log-view_warp">
    <a-layout>
      <div class="kf-log-view__warp">
        <KfDashboard @boardSizeChange="handleChangeBoardSize">
          <template #title>
            <KfDashboardItem v-if="props.type && props.type === 'replay'">
              <div class="replay_title">
                {{ $t('replay.replay') }}
              </div>
            </KfDashboardItem>
            <KfDashboardItem v-if="props.type && props.type === 'replay'">
              <div class="replay_title">
                {{
                  `${$t('replay.log_level')}: ${
                    LogLevelType[props.params.logLevel.replace('%20', ' ')] ||
                    'INFO'
                  }`
                }}
              </div>
            </KfDashboardItem>
            <KfDashboardItem v-if="props.type && props.type === 'replay'">
              <div class="replay_title">
                {{ `${$t('replay.begin_time')}: ${props.params.beginTime}` }}
              </div>
            </KfDashboardItem>
            <KfDashboardItem v-if="props.type && props.type === 'replay'">
              <div class="replay_title">
                {{ `${$t('replay.end_time')}: ${props.params.endTime}` }}
              </div>
            </KfDashboardItem>
          </template>
          <template #header>
            <KfDashboardItem>
              <a-button
                v-if="props.type && props.type === 'replay'"
                @click="reLoadLog"
                size="small"
                :loading="isLoading"
              >
                {{ $t('replay.try_again') }}
              </a-button>
            </KfDashboardItem>
            <KfDashboardItem>
              <a-checkbox
                v-model:checked="scrollToBottomChecked"
                size="small"
                @change="scrollToBottom"
              >
                {{ $t('logview.scroll_to_bottom') }}
              </a-checkbox>
            </KfDashboardItem>
            <KfDashboardItem>
              <div class="search-in-table__warp">
                <a-input-search
                  ref="inputSearchRef"
                  v-model:value="searchKeyword"
                  class="search-int-table__item"
                  :placeholder="$t('keyword_input')"
                  style="width: 120px"
                />
                <div class="current-to-total search-int-table__item">
                  {{ currentResultIndex }} /
                  {{ totalResultCount }}
                </div>
                <div
                  class="find-up-down search-int-table__item kf-actions__warp"
                >
                  <up-outlined
                    style="font-size: 14px; margin-left: 0px"
                    @click="handleToUpSearchResult"
                  />
                  <down-outlined
                    style="font-size: 14px; margin-left: 8px"
                    @click="handleToDownSearchResult"
                  />
                </div>
              </div>
            </KfDashboardItem>
            <KfDashboardItem>
              <a-button size="small">
                <template #icon>
                  <ReloadOutlined class="kf-hover" style="font-size: 14px" />
                </template>
              </a-button>
            </KfDashboardItem>
            <KfDashboardItem>
              <a-button size="small" @click="handleOpenFileLocation">
                {{ $t('folder') }}
              </a-button>
            </KfDashboardItem>
            <KfDashboardItem>
              <a-button size="small" type="primary" @click="handleRemoveLog">
                {{ $t('clean') }}
              </a-button>
            </KfDashboardItem>
          </template>

          <DynamicScroller
            id="kf-log-table"
            ref="scrollerTableRef"
            class="kf-table"
            :items="logList.list"
            :min-item-size="36"
            :simple-array="true"
          >
            <template
              #default="{
                item,
                index,
                active,
              }: {
                item: KungfuApi.KfLogData,
                index: number,
                active: boolean,
              }"
            >
              <DynamicScrollerItem
                :item="item"
                :active="active"
                :size-dependencies="[item.message]"
                :data-index="index"
              >
              <div
                :id="`kf-log-item-${item.id}`"
                :active="active"
                class="kf-log-line"
                v-html="dealLogMessage(getItemHtmlResult(item, 'message'))"
              ></div>
              </DynamicScrollerItem>
            </template>
          </DynamicScroller>
        </KfDashboard>
      </div>
    </a-layout>
  </div>
</template>

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

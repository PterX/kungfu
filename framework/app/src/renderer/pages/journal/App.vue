<template>
  <a-layout>
    <div class="kf-journal-view__wrap">
      <div class="kf-journal-head-warp" :style="journalHeadStyle">
        <div
          v-if="!visualVisible"
          class="kf-journal-session__warp kf-translateZ"
        >
          <KfDashboard @boardSizeChange="handleBodySizeChange">
            <template #header>
              <KfDashboardItem>
                <a-input-search
                  v-model:value="searchKeyword"
                  :placeholder="$t('keyword_input')"
                  style="width: 120px"
                />
              </KfDashboardItem>
              <KfDashboardItem>
                <a-button size="small" @click="setSessions">
                  <template #icon>
                    <reload-outlined style="font-size: 14px"></reload-outlined>
                  </template>
                </a-button>
              </KfDashboardItem>
            </template>
            <a-table
              class="kf-ant-table"
              style="height: 100% !important"
              :columns="columns"
              :data-source="tableData"
              :pagination="false"
              size="small"
              :row-class-name="dealRowClassName"
              :custom-row="customRow"
              :default-expand-all-rows="true"
              :scroll="{ y: dashboardBodyHeight - 4 }"
            >
              <template #emptyText>
                <a-empty
                  :image="simpleImage"
                  :description="t('empty_text')"
                ></a-empty>
              </template>
              <template
                #bodyCell="{
                  column,
                  record,
                }: {
                  column: KfTradingDataTableHeaderConfig,
                  record: KungfuApi.SessionResolved,
                }"
              >
                <template v-if="column.dataIndex === 'sessionName'">
                  <div class="session-name__warp">
                    <a-tag
                      :color="dealCategory(record.category)?.color || 'default'"
                    >
                      {{ dealCategory(record.category)?.name }}
                    </a-tag>
                    <span>
                      {{
                        record[
                          column.dataIndex as keyof KungfuApi.SessionResolved
                        ]
                      }}
                    </span>
                  </div>
                </template>
                <template v-else-if="column.dataIndex === 'status'">
                  <span
                    :style="{
                      color: SessionStatus[record[column.dataIndex]].color,
                    }"
                  >
                    {{ SessionStatus[record[column.dataIndex]].name }}
                  </span>
                </template>
              </template>
            </a-table>
          </KfDashboard>
        </div>
        <div v-if="visualVisible" class="kf-journal-visualization">
          <EntryVisualization
            ref="entryVisualzationRef"
            :category="currentSession?.category"
          />
        </div>
      </div>
      <div class="gutter" @mousedown="mouseDownHandler"></div>
      <div class="kf-journal-content" :style="journalContentStyle">
        <div class="kf-journal-control-bar">
          <div class="kf-journal-bar-title" v-if="currentSession">
            <a-tag :color="currentCategoryData?.color || 'default'">
              {{ currentCategoryData?.name }}
            </a-tag>
            <span>
              {{ currentSessionName }}
            </span>
          </div>
          <TimeSlider
            v-if="currentSession"
            :step="60"
            class="kf-journal-time-slider"
          ></TimeSlider>
          <JournalActions
            :is-show-replay-action="isShowReplayAction || false"
            :is-show-visual-action="isShowVisualAction"
            @export-journal-data="onJournalActionsData"
            @start-replay="dealLocation"
            @show-visual="onEntryVisualization"
          />
        </div>
        <div class="kf-journal-menu__wrap">
          <a-menu
            v-model:selectedKeys="currentMenuList"
            class="kf-journal-menu-tab"
          >
            <a-menu-item v-for="item in menus" :key="item.key">
              <template #icon>
                <component :is="item.icon"></component>
              </template>
              {{ item.title }}
            </a-menu-item>
          </a-menu>
          <div class="kf-journal-menu-content">
            <EventsDashBoard
              v-show="currentSession && isCurrentMenuItem('event')"
              ref="eventDashBoard"
            />
            <template v-if="replayParams.logPath">
              <Replay
                v-show="
                  currentSession &&
                  isShowReplayAction &&
                  replayParams.logPath &&
                  replayParams.processId &&
                  isCurrentMenuItem('replay')
                "
                ref="replayRef"
                :params="replayParams"
                @stop-replay-loading="stopLoadingInterval"
              />
            </template>
          </div>
        </div>
      </div>
    </div>
  </a-layout>
  <KfReplaySettingModal
    v-if="setReplayModalVisible"
    :width="720"
    v-model:visible="setReplayModalVisible"
    :is-journal="true"
    :can-backtest="canBacktest"
    :session-options="sessionOptions"
    :session-info="replayConfig.session_info"
    :begin-time="replayConfig.begin_time"
    :end-time="replayConfig.end_time ? replayConfig.end_time : ''"
    :now="formatSessionTime(BigInt(new Date().getTime()) * 1000000n)"
    :log-level="replayConfig.log_level"
    @close="setReplayModalVisible = false"
    @confirm="
      async (event) => (
        await handleReplayModal(event, true),
        startLoadingInterval(),
        updateLogLevel()
      )
    "
  ></KfReplaySettingModal>
</template>

<script setup lang="ts">
import { onMounted, ref, computed, watch, onUnmounted, ComputedRef } from 'vue';
import { ensureFileSync, outputFile } from 'fs-extra';
import { storeToRefs } from 'pinia';
import { getSessionColumns, SessionStatus } from './config';
import { getCurrentWindow } from '@electron/remote';
import {
  buildProcessReplayPath,
  buildProcessBacktestPath,
} from '@kungfu-trader/kungfu-js-api/config/pathConfig';
import {
  messagePrompt,
  removeLoadingMask,
  useDashboardBodySize,
  useTableSearchKeyword,
} from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/uiUtils';
import {
  getProcessIdByKfLocation,
  getYearMonthDay,
  delayMilliSeconds,
} from '@kungfu-trader/kungfu-js-api/utils/commonUtils';
import { listProcessStatus } from '@kungfu-trader/kungfu-js-api/utils/processUtils';

import { dealCategory } from './utils';
import { Empty } from 'ant-design-vue';

import {
  UnorderedListOutlined,
  HistoryOutlined,
  ReloadOutlined,
} from '@ant-design/icons-vue';
import TimeSlider from './components/TimeSlider.vue';
import JournalActions from './components/JournalActions.vue';
import EventsDashBoard from './components/EventsDashboard.vue';
import EntryVisualization from './components/EntryVisualization.vue';
import { useJournalStore } from './store/journalStore';
import VueI18n from '@kungfu-trader/kungfu-js-api/language';
import { getAllKfConfigOriginData } from '@kungfu-trader/kungfu-js-api/actions';
import { ipcEmit } from '@kungfu-trader/kungfu-app/src/renderer/ipcMsg/emitter';
import { useGlobalStore } from '@kungfu-trader/kungfu-app/src/renderer/pages/index/store/global';

import { useReplay } from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/actionsUtils';
import KfDashboard from '../../components/public/KfDashboard.vue';

import KfDashboardItem from '@kungfu-trader/kungfu-app/src/renderer/components/public/KfDashboardItem.vue';
import Replay from '@kungfu-trader/kungfu-app/src/renderer/pages/replay/Replay.vue';
import KfReplaySettingModal from '@kungfu-trader/kungfu-app/src/renderer/components/public/KfReplaySettingModal.vue';

const { t } = VueI18n.global;
const { testCase } = storeToRefs(useGlobalStore());
const replayRef = ref();

let currentWindow: Electron.BrowserWindow | null = null;
const {
  sessions,
  currentSession,
  currentSessionName,
  currentSessionKey,
  currentCategoryData,
  currentFrameList,
} = storeToRefs(useJournalStore());

const {
  replayConfig,
  setReplayModalVisible,
  sessionOptions,
  formatSessionTime,
  handleOpenReplayConfirmView,
  journalReplayflag,
  replayProcessParams,
  handleReplayModal,
  startLoadingInterval,
  stopLoadingInterval,
  replayPreLoading,
} = useReplay();

const { setSessions, setCurrentSession } = useJournalStore();
const { handleBodySizeChange, dashboardBodyHeight } = useDashboardBodySize();
const columns = getSessionColumns();

const entryVisualzationRef = ref();

const operator = ref<KungfuApi.KfConfig[]>([]);
const strategy = ref<KungfuApi.KfConfig[]>([]);
const td = ref<KungfuApi.KfConfig[]>([]);
const canBacktest = computed(() => {
  return currentSession.value?.category === 'strategy';
});
const replayParams = computed(() => {
  const currentSessionValue = currentSession.value;
  const replayConfigValue = replayConfig.value;
  const replayEnabled = testCase.value.replayEnabled;
  if (
    !(
      currentSessionValue &&
      (replayEnabled[currentSessionValue.category] ||
        (currentSessionValue.category === 'system' &&
          currentSessionValue.name === 'ledger'))
    )
  ) {
    return {};
  }
  const mode = replayConfigValue.enable_matcher ? 'backtest' : 'replay';
  const { category, group, name, begin_time, end_time } = currentSessionValue;
  const dateStr = getYearMonthDay();
  const logPath = setReplayModalVisible.value
    ? replayParams.value.logPath
    : replayConfigValue.enable_matcher
    ? buildProcessBacktestPath(
        { category, group, name, mode },
        `${name}_${dateStr}`,
      )
    : buildProcessReplayPath(
        { category, group, name, mode },
        `${name}_${dateStr}`,
      );
  const beginTime =
    replayConfigValue.begin_time || formatSessionTime(begin_time);
  const endTime =
    replayConfigValue.end_time ||
    (end_time
      ? formatSessionTime(end_time)
      : formatSessionTime(BigInt(new Date().getTime()) * 1000000n));
  const processId = getProcessIdByKfLocation({ category, group, name, mode });
  const enableMatcher = replayConfigValue.enable_matcher || false;

  return {
    category,
    group,
    beginTime,
    endTime,
    logPath,
    logLevel: replayConfigValue.log_level || '-l info',
    sessionName: name || '',
    filePath: replayConfigValue.file_path || '',
    processId,
    enableMatcher: enableMatcher.toString(),
    replayPreLoading: replayPreLoading.value,
  };
});
const isShowReplayAction: ComputedRef<boolean> = computed(() => {
  return (
    currentSession.value &&
    (testCase.value.replayEnabled[currentSession.value.category] ||
      (currentSession.value.category === 'system' &&
        currentSession.value.name === 'ledger'))
  );
});
const isShowVisualAction = computed(() => {
  return !!currentSession.value && currentSession.value.category === 'strategy';
});
const { searchKeyword, tableData } =
  useTableSearchKeyword<KungfuApi.SessionResolved>(sessions, [
    'sessionName',
    'category',
    'group',
    'name',
  ]);

const simpleImage = Empty.PRESENTED_IMAGE_SIMPLE;
const currentMenuList = ref<('event' | 'visual' | 'replay')[]>(['event']);
const menus = computed(() => [
  ...(isShowReplayAction.value
    ? [
        {
          key: 'event',
          title: t('journalConfig.Event'),
          icon: UnorderedListOutlined,
        },
        {
          key: 'replay',
          title: t('journalConfig.replay'),
          icon: HistoryOutlined,
        },
      ]
    : [
        {
          key: 'event',
          title: t('journalConfig.Event'),
          icon: UnorderedListOutlined,
        },
      ]),
]);
const isCurrentMenuItem = (key: 'event' | 'visual' | 'replay') => {
  if (currentMenuList.value.includes(key) && key === 'replay') {
    return (
      currentSession.value &&
      (testCase.value.replayEnabled[currentSession.value.category] ||
        (currentSession.value.category === 'system' &&
          currentSession.value.name === 'ledger'))
    );
  } else {
    return currentMenuList.value.includes(key);
  }
};
const visualVisible = ref<boolean>(false);
const boardStyle = localStorage.getItem('boardStyle')
  ? JSON.parse(localStorage.getItem('boardStyle') as string)
  : {};

const journalHeadStyle = ref<KungfuApi.BoardStyle>(
  boardStyle['journalHead'] || {
    height: '20%',
  },
);
const journalContentStyle = ref<KungfuApi.BoardStyle>(
  boardStyle['journalContent'] || {
    height: '80%',
  },
);

const exportFileName = computed(() => {
  if (currentSession.value) {
    return `${
      currentSession.value.sessionName
    }_${currentSession.value.beginTimeResolved
      .split('.')[0]
      .split(':')
      .join('-')}`;
  }

  return 'session';
});

const customRow = (record: KungfuApi.SessionResolved) => {
  return {
    onClick: async () => {
      setCurrentSession(record);

      if (replayParams.value.processId) {
        stopLoadingInterval();
        const config = localStorage.getItem('replaySetting');
        const replaySetting = config ? JSON.parse(config) : {};
        replayConfig.value = {
          session_info: '',
          category: '',
          group: 'default',
          begin_time: '',
          end_time: '',
          log_level: replaySetting.log_level || '-l info',
          session_name: '',
          file_path: '',
          enable_matcher: false,
        };

        updateLogLevel();
      }
    },
  };
};

const mouseMoveHandler = (event: MouseEvent) => {
  const journalHeadDom = ref<HTMLElement | null>(
    document.querySelector('.kf-journal-head-warp'),
  );
  const journalContentDom = ref<HTMLElement | null>(
    document.querySelector('.kf-journal-content'),
  );

  if (!journalHeadDom.value || !journalContentDom.value) return;

  const container = document.querySelector('.kf-journal-view__wrap');
  if (!container) return;
  const div = container.getBoundingClientRect();

  const leftHeight = event.clientY;
  const rightHeight = window.innerHeight - event.clientY - 5;

  journalHeadStyle.value = {
    height: `${(100 * leftHeight) / div.height}%`,
    flex: 'unset',
  };
  journalContentStyle.value = {
    height: `${(100 * rightHeight) / div.height}%`,
    flex: 'unset',
  };
};
const mouseUpHandler = () => {
  if (visualVisible.value) {
    entryVisualzationRef.value?.handleResize(true);
  }
  localStorage.setItem(
    'boardStyle',
    JSON.stringify({
      journalHead: journalHeadStyle.value,
      journalContent: journalContentStyle.value,
    }),
  );

  document.removeEventListener('mousemove', mouseMoveHandler);
  document.removeEventListener('mouseup', mouseUpHandler);
};
const mouseDownHandler = (event: MouseEvent) => {
  document.addEventListener('mousemove', mouseMoveHandler);
  document.addEventListener('mouseup', mouseUpHandler);
};

onMounted(async () => {
  currentWindow = getCurrentWindow();
  const {
    operator: originOperator,
    strategy: originStategy,
    td: originTd,
  } = await getAllKfConfigOriginData();
  operator.value = originOperator;
  strategy.value = originStategy;
  td.value = originTd;

  await setSessions();
  removeLoadingMask();
});

onUnmounted(() => {
  currentWindow?.destroy();
});

watch(
  () => journalReplayflag.value,
  (val) => {
    if (val) {
      if (currentSession.value) {
        const dateStr = getYearMonthDay();
        const location = {
          category: currentSession.value.category,
          group: currentSession.value.group,
          name: currentSession.value.name,
          mode: replayConfig.value.enable_matcher ? 'backtest' : 'replay',
        };
        const logPath = replayConfig.value.enable_matcher
          ? buildProcessBacktestPath(location, `${location.name}_${dateStr}`)
          : buildProcessReplayPath(location, `${location.name}_${dateStr}`);

        ensureFileSync(logPath);
        outputFile(logPath, '')
          .then(() => {
            if (currentWindow) {
              ipcEmit('clear-process', {
                processId: (replayParams.value.processId || '') as string,
              })
                .then(() => {
                  const pawin =
                    currentWindow && currentWindow.getParentWindow();
                  if (pawin) {
                    pawin.webContents.send('startReplay', {
                      replayProcessParams: replayProcessParams.value,
                    });
                    currentMenuList.value = ['replay'];
                  }
                })
                .catch((err) => {
                  console.error(err);
                });
            }
          })
          .catch((err) => {
            console.log(err);
          });
      }
    }
  },
);

function updateLogLevel() {
  delayMilliSeconds(0).then(() => {
    replayRef.value?.updateLogLevel();
  });
}

const onJournalActionsData = (
  exportData: (fileName: string, exportData: KungfuApi.FrameResolved[]) => void,
) => {
  exportData(exportFileName.value, currentFrameList.value);
};
const dealLocation = async () => {
  const { value: currentSessionValue } = currentSession;
  if (!currentSessionValue) {
    messagePrompt().error(t('replay.please_select_session'));
    return;
  }

  const { category, group, name, mode, location_uid } = currentSessionValue;
  const locationResolved = {
    category,
    group,
    name,
    mode,
    location_uid,
    value: '',
  };

  const { processStatusWithDetail } = await listProcessStatus();
  const processId = getProcessIdByKfLocation({
    category,
    group,
    name,
    mode,
  });

  const processStatusDetail = processStatusWithDetail[processId];
  if (!processStatusDetail && name === 'strategy' && group !== 'default') {
    messagePrompt().error(t('replay.process_not_found'));
    return;
  }

  switch (category) {
    case 'operator':
      if (group === 'default') {
        const operatorValue = operator.value;
        if (operatorValue.length === 0) {
          messagePrompt().error(t('replay.process_has_not_been_started'));
          return;
        }
        const operatorMatch = operatorValue.find(
          (item) => item.location_uid === location_uid,
        );
        if (operatorMatch) {
          locationResolved.value = operatorMatch.value;
        }
      }
      break;
    case 'strategy':
      if (group === 'default') {
        const strategyValue = strategy.value;
        if (strategyValue.length === 0) {
          messagePrompt().error(t('replay.process_has_not_been_started'));
          return;
        }
        const strategyMatch = strategyValue.find(
          (item) => item.location_uid === location_uid,
        );
        if (strategyMatch) {
          locationResolved.value = strategyMatch.value;
        }
      }
      break;
    case 'td':
      if (td.value.length === 0) {
        messagePrompt().error(t('replay.process_has_not_been_started'));
        return;
      }
      break;
    case 'system':
      if (name !== 'ledger') {
        messagePrompt().error(t('replay.process_can_not_replay'));
        return;
      }
      break;
    default:
      messagePrompt().error(t('replay.process_can_not_replay'));
      return;
  }

  handleOpenReplayConfirmView(locationResolved, currentSessionValue);
};
const dealRowClassName = (row) => {
  return row.begin_time === currentSessionKey.value
    ? 'current-global-kfLocation'
    : '';
};

function onEntryVisualization(visible: boolean) {
  visualVisible.value = visible;
}
</script>

<style lang="less">
@import '@kungfu-trader/kungfu-app/src/renderer/assets/less/coverAnt.less';
@import '@kungfu-trader/kungfu-app/src/renderer/assets/less/base.less';
@import '@kungfu-trader/kungfu-app/src/renderer/assets/less/public.less';
@import '@kungfu-trader/kungfu-app/src/renderer/assets/less/variables.less';

#app {
  width: 100%;
  height: 100%;
  -webkit-font-smoothing: antialiased;
  -moz-osx-font-smoothing: grayscale;
  text-align: center;

  .default-log-view_warp .ant-layout .kf-dashboard__body {
    background-color: transparent;
  }

  .ant-layout {
    height: 100%;
    background: @component-background;

    .kf-journal-view__wrap {
      height: 100%;
      width: 100%;
      padding: 0 8px 8px 8px;
      display: flex;
      flex-direction: column;

      .gutter {
        cursor: row-resize;
        width: 100%;
        height: 4px !important;
        flex: 0 0 4px;
      }

      .gutter:hover {
        background-color: #333;
      }

      .gutter:active {
        background-color: #333;
      }

      .kf-journal-content {
        flex: 1 1 80%;
        width: 100%;
        display: flex;
        flex-direction: column;
        z-index: 99;
      }
      .kf-journal-head-warp {
        flex: 1 1 20%;
        width: 100%;

        .kf-journal-session__warp {
          width: 60%;
          height: 100%;
          margin: auto;
          padding-top: 8px;
          box-sizing: border-box;

          .ant-empty {
            height: auto;
            margin-top: 48px;
          }
        }
        .session-name__warp {
          word-break: break-all;
        }

        .kf-journal-visualization {
          width: 100%;
          height: 100%;
          padding-top: 8px;
          box-sizing: border-box;
          z-index: 1;
        }
      }

      .kf-journal-control-bar {
        flex: 0 0 50px;
        height: 50px;
        background-color: #1d1d1d;
        padding: 5px 16px;
        margin-bottom: 2px;
        display: flex;
        align-items: center;
        justify-content: space-between;

        .kf-journal-bar-title {
          max-width: 300px;
          font-size: 14px;
          margin-right: 16px;
          white-space: nowrap;
          overflow: hidden;
          text-overflow: ellipsis;
        }

        .kf-journal-time-slider {
          flex: 0 1 560px;
        }
      }

      .kf-journal-menu__wrap {
        width: 100%;
        height: calc(100% - 350px);
        flex: auto;

        display: flex;

        .kf-journal-menu-tab {
          flex: 0 0 120px;
          width: 120px;
          margin-right: 2px;

          li {
            width: 100%;
          }
        }

        .kf-journal-menu-content {
          flex: auto;
          height: 100%;
          width: 100%;
        }
      }
    }
  }
}
</style>

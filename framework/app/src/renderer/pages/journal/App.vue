<template>
  <a-layout>
    <div class="kf-journal-view__wrap">
      <div class="kf-journal-session__warp kf-translateZ">
        <KfDashboard @boardSizeChange="handleBodySizeChange">
          <template v-slot:header>
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
            :columns="columns"
            :data-source="tableData"
            :pagination="false"
            size="small"
            :row-class-name="dealRowClassName"
            :custom-row="customRow"
            :default-expand-all-rows="true"
            :scroll="{ y: dashboardBodyHeight - 4 }"
            :emptyText="$t('empty_text')"
          >
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
                <a-tag
                  :color="dealCategory(record.category)?.color || 'default'"
                >
                  {{ dealCategory(record.category)?.name }}
                </a-tag>
                {{
                  record[column.dataIndex as keyof KungfuApi.SessionResolved]
                }}
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

      <div class="kf-journal-control-bar">
        <div class="kf-journal-bar-title" v-if="currentSession">
          <a-tag :color="currentCategoryData?.color || 'default'">
            {{ currentCategoryData?.name }}
          </a-tag>
          {{ currentSessionName }}
        </div>
        <TimeSlider
          v-if="currentSession"
          :step="60"
          class="kf-journal-time-slider"
        ></TimeSlider>
        <JournalActions
          :currentSession="currentSession"
          @export-journal-data="onJournalActionsData"
          @start-replay="dealLocation"
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
            v-if="currentSession"
            v-show="isCurrentMenuItem('event')"
            ref="eventDashBoard"
          />
          <Replay
            v-if="
              currentSession &&
              replayPramas.processId &&
              isCurrentMenuItem('replay')
            "
            :category="currentSession.category"
            :params="replayPramas"
            :is-journal="true"
            :replayProcessParams="replayProcessParams"
            :type="'replay'"
            :key="replayPramas.processId"
          />
        </div>
      </div>
    </div>
  </a-layout>
  <ReplayForm
    v-if="setReplayModalVisible"
    :width="520"
    v-model:visible="setReplayModalVisible"
    :session-options="sessionOptions"
    :session-info="replayConfig.session_info"
    :begin-time="replayConfig.begin_time.split(' ')[1]"
    :end-time="replayConfig.end_time ? replayConfig.end_time.split(' ')[1] : ''"
    :now="getNanoDateString(BigInt(new Date().getTime()) * 1000000n)"
    :log-level="replayConfig.log_level"
    @close="setReplayModalVisible = false"
    @confirm="(event) => handleReplayModal(event, currentLocationConfig, true)"
  ></ReplayForm>
</template>

<script setup lang="ts">
import {
  onMounted,
  ref,
  computed,
  getCurrentInstance,
  watch,
  onUnmounted,
} from 'vue';
import { storeToRefs } from 'pinia';
import { getSessionColumns, SessionStatus } from './config';
import { BrowserWindow } from '@electron/remote';
import { buildProcessReplayPath } from '@kungfu-trader/kungfu-js-api/config/pathConfig';
import {
  messagePrompt,
  removeLoadingMask,
  useDashboardBodySize,
  useTableSearchKeyword,
  setHtmlTitle,
} from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/uiUtils';
import { getYearMonthDay } from '@kungfu-trader/kungfu-js-api/utils/busiUtils';
import { getNanoDateString } from '@kungfu-trader/kungfu-js-api/kungfu';

import { dealCategory } from './utils';
import { UnorderedListOutlined, HistoryOutlined } from '@ant-design/icons-vue';
import TimeSlider from './components/TimeSlider.vue';
import JournalActions from './components/JournalActions.vue';
import EventsDashBoard from './components/EventsDashboard.vue';
import { useJournalStore } from './store/journalStore';
import VueI18n from '@kungfu-trader/kungfu-js-api/language';
import { getAllKfConfigOriginData } from '@kungfu-trader/kungfu-js-api/actions';

import { useReplay } from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/actionsUtils';
import KfDashboard from '../../components/public/KfDashboard.vue';

import KfDashboardItem from '@kungfu-trader/kungfu-app/src/renderer/components/public/KfDashboardItem.vue';
import Replay from '@kungfu-trader/kungfu-app/src/renderer/pages/logview/index.vue';
import ReplayForm from '@kungfu-trader/kungfu-app/src/components/modules/strategy/ReplayForm.vue';

const { t } = VueI18n.global;

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
  currentLocation: currentLocationConfig,
  replayConfig,
  setReplayModalVisible,
  sessionOptions,
  handleOpenReplayConfirmView,
  journalReplayflag,
  replayProcessParams,
  handleReplayModal,
} = useReplay();

const { setSessions, setCurrentSession } = useJournalStore();
const { handleBodySizeChange, dashboardBodyHeight } = useDashboardBodySize();
const columns = getSessionColumns();
const operator = ref<KungfuApi.KfConfig[]>([]);
const strategy = ref<KungfuApi.KfConfig[]>([]);
const replayList = ['strategy', 'operator'];
const replayPramas = computed(() => {
  if (
    !currentSession.value ||
    !replayList.includes(currentSession.value.category)
  )
    return {};
  const { category, group, name } = currentSession.value;
  const dateStr = getYearMonthDay();
  const logPath = buildProcessReplayPath(
    {
      category,
      group,
      name,
    },
    `${currentSession.value.name}_${dateStr}`,
  );
  const begin_time =
    replayConfig.value.begin_time.split(' ')[1] ||
    getNanoDateString(currentSession.value.begin_time);
  const end_time =
    replayConfig.value.end_time.split(' ')[1] ||
    (currentSession.value.end_time
      ? getNanoDateString(currentSession.value.end_time)
      : getNanoDateString(BigInt(new Date().getTime()) * 1000000n));
  return {
    beginTime: begin_time,

    endTime: end_time,

    logPath: logPath,
    logLevel: replayConfig.value.log_level || '-l info',
    processId: `${category}_replay_${begin_time}_${end_time}`,
  };
});

const { searchKeyword, tableData } =
  useTableSearchKeyword<KungfuApi.SessionResolved>(sessions, [
    'sessionName',
    'category',
    'group',
    'name',
  ]);

const app = getCurrentInstance();
const currentMenuList = ref<('event' | 'visual' | 'replay')[]>(['event']);
const menus = [
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
];
const isCurrentMenuItem = (key: 'event' | 'visual' | 'replay') => {
  if (currentMenuList.value.includes(key) && key === 'replay') {
    setHtmlTitle(replayPramas.value.logPath);
    return replayList.includes(extractWordAfterLog(replayPramas.value.logPath));
  } else {
    return currentMenuList.value.includes(key);
  }
};

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
    onClick: () => {
      setCurrentSession(record);
      replayConfig.value = {
        session_info: '',
        group: 'default',
        begin_time: '',
        end_time: '',
        log_level: '',
        session_name: '',
        path: '',
      };
    },
  };
};

onMounted(async () => {
  currentWindow = BrowserWindow.getFocusedWindow();
  const { operator: originOperator, strategy: originStategy } =
    await getAllKfConfigOriginData();
  operator.value = originOperator;
  strategy.value = originStategy;
  setSessions();
  removeLoadingMask();
  window.addEventListener('resize', () => {
    app?.proxy &&
      app?.proxy.$globalBus.next({
        tag: 'resize',
      } as KfEvent.ResizeEvent);
  });
});

onUnmounted(() => {
  currentWindow?.destroy();
});

watch(
  () => journalReplayflag.value,
  (val) => {
    if (val) {
      if (currentWindow) {
        const pawin = currentWindow.getParentWindow();
        if (pawin) {
          pawin.webContents.send('startReplay', {
            replayProcessParams: replayProcessParams.value,
          });
          currentMenuList.value = ['replay'];
        }
      }
    }
  },
);

const onJournalActionsData = (
  exportData: (fileName: string, exportData: KungfuApi.FrameResolved[]) => void,
) => {
  exportData(exportFileName.value, currentFrameList.value);
};
const dealLocation = () => {
  if (!currentSession.value) {
    messagePrompt().error(t('replay.please_select_session'));
    return;
  }
  const locationResolved: KungfuApi.KfConfig = {
    category: currentSession.value.category,
    group: currentSession.value.group,
    name: currentSession.value.name,
    mode: currentSession.value.mode,
    location_uid: currentSession.value.location_uid,
    value: '',
  };

  if (currentSession.value?.category === 'operator') {
    if (operator.value.length === 0) {
      messagePrompt().error(t('strategyConfig.operator_be_empty'));
      return;
    }
    for (let i = 0; i < operator.value.length; i++) {
      if (
        operator.value[i].location_uid === currentSession.value.location_uid
      ) {
        locationResolved.value = operator.value[i].value;
        break;
      }
    }
  } else if (currentSession.value?.category === 'strategy') {
    if (strategy.value.length === 0) {
      messagePrompt().error();
      return;
    }
    for (let i = 0; i < strategy.value.length; i++) {
      if (
        strategy.value[i].location_uid === currentSession.value.location_uid
      ) {
        locationResolved.value = strategy.value[i].value;
        break;
      }
    }
  } else {
    messagePrompt().error(
      t('replay.only_operator_or_strategy_can_be_replayed'),
    );
    return;
  }

  handleOpenReplayConfirmView(locationResolved, currentSession.value);
};

const dealRowClassName = (row) => {
  return row.begin_time === currentSessionKey.value
    ? 'current-global-kfLocation'
    : '';
};

function extractWordAfterLog(inputString) {
  const match = inputString.match(/\/log\/([^\/]+)/);
  return match ? match[1] : null;
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

      .kf-journal-session__warp {
        flex: 0 0 300px;
        height: 300px;
        width: 60%;
        margin: auto;
        padding: 8px 0;
        box-sizing: border-box;
      }

      .kf-journal-control-bar {
        flex: 0 0 50px;
        height: 50px;
        background-color: #1d1d1d;
        padding: 5px 20px;
        margin-bottom: 2px;
        display: flex;
        align-items: center;
        justify-content: space-between;

        .kf-journal-bar-title {
          font-size: 14px;
          margin-right: 16px;
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

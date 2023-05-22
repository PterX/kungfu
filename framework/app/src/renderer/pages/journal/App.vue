<template>
  <a-layout>
    <div class="kf-journal-view__wrap">
      <div class="kf-journal-session__wrap">
        <KfTradingDataTable
          :data-source="sessions"
          :columns="sessionColumns"
          key-field="begin_time"
          :resizable="false"
          :custom-row-class="dealRowClassName"
          @click-cell="handleSelectSession"
        >
          <template
            #default="{
              item,
              column,
            }: {
              item: KungfuApi.SessionResolved,
              column: KfTradingDataTableHeaderConfig,
            }"
          >
            <template v-if="column.dataIndex === 'session_id_resolved'">
              <a-tag :color="dealCategory(item.category)?.color || 'default'">
                {{ dealCategory(item.category)?.name }}
              </a-tag>
              {{ item[column.dataIndex as keyof KungfuApi.SessionResolved] }}
            </template>
            <template v-else-if="column.dataIndex === 'status'">
              <span
                :style="{ color: SessionStatus[item[column.dataIndex]].color }"
              >
                {{ SessionStatus[item[column.dataIndex]].name }}
              </span>
            </template>
          </template>
        </KfTradingDataTable>
      </div>
      <div class="kf-journal-control-bar">
        <div class="kf-journal-bar-title">
          <a-tag :color="currentCategoryData?.color || 'default'">
            {{ currentCategoryData?.name }}
          </a-tag>
          {{ currentSessionTitle }}
        </div>
        <TimeSlider
          ref="timeSlider"
          v-model:current-time="currentTime"
          :current-session="currentSession"
          :now-time="nowTime"
          :begin-time="beginTime"
          :end-time="endTime"
          :step="60"
          class="kf-journal-time-slider"
        ></TimeSlider>
        <ExportJournal @export-journal-data="onExportJournalData" />
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
            v-show="isCurrentMenuItem('event')"
            ref="eventDashBoard"
            :current-session="currentSession"
            :begin-time="beginTime"
            :end-time="endTime"
            :now-time="nowTime"
            :current-time="currentTime"
            :location-map="SourceAndDestNameMap"
            @update-current-time="onUpdateCurrentTime"
          />
        </div>
      </div>
    </div>
  </a-layout>
</template>

<script setup lang="ts">
import {
  onMounted,
  ref,
  computed,
  toRaw,
  watch,
  nextTick,
  watchEffect,
  getCurrentInstance,
} from 'vue';
import {
  dealKfTime,
  sessionStore,
  io,
} from '@kungfu-trader/kungfu-js-api/kungfu';
import { getSessionColumns, SessionStatus } from './config';
import { removeLoadingMask } from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/uiUtils';

import {
  getCurrentLocation,
  dealCategory,
  dealSessionsToMap,
  getAbs,
} from './utils';
import { setTimerPromiseTask } from '@kungfu-trader/kungfu-js-api/utils/busiUtils';
import { SessionStatusEnum } from '@kungfu-trader/kungfu-js-api/typings/enums';
import { UnorderedListOutlined } from '@ant-design/icons-vue';
import KfTradingDataTable from '@kungfu-trader/kungfu-app/src/renderer/components/public/KfTradingDataTable.vue';
import TimeSlider from './components/TimeSlider.vue';
import ExportJournal from './components/ExportJournal.vue';
import EventsDashBoard from './components/EventsDashboard.vue';
import { useJournalStore } from './store/journalStore';
import VueI18n from '@kungfu-trader/kungfu-js-api/language';

type LocationRseolved = KungfuApi.KfLocation & {
  uname: string;
  uid: number;
};
const { t } = VueI18n.global;
const beginTime = ref<bigint>(BigInt(new Date().getTime()) * 1000000n);
const endTime = ref<bigint>(0n);
const currentLocation = getCurrentLocation();
const timeSlider = ref();
const eventDashBoard = ref();
const mdSession = ref();
const journalStore = useJournalStore();
const allLocations = ref<Record<string, LocationRseolved>>({});
const sessionsMap = ref<Record<string, KungfuApi.SessionResolved>>({});
const SourceAndDestNameMap = ref<Record<string, string>>({});
const sessions = computed(() => {
  return Object.values(sessionsMap.value);
});
const runningSessions = computed(() => {
  return sessions.value.filter(
    (item) => item.status === SessionStatusEnum.Running,
  );
});

const nowTime = ref(BigInt(new Date().getTime()) * 1000000n);
const currentTime = ref(BigInt(new Date().getTime()) * 1000000n);
const app = getCurrentInstance();

watchEffect(() => {
  const updateNowTime = () => {
    nowTime.value = BigInt(new Date().getTime()) * 1000000n;
    setTimeout(updateNowTime, 1000);
  };

  updateNowTime();
});
const currentSessionKey = ref('');
const currentSessionId = ref(-1);

const currentSession = computed(() => {
  if (currentSessionKey.value && Object.keys(sessionsMap.value).length) {
    return sessionsMap.value[currentSessionKey.value];
  }

  return null;
});

const currentSessionTitle = computed(() => {
  if (currentSession.value) {
    return `${currentSession.value.session_id_resolved}`;
  }

  return '';
});

const currentCategoryData = computed(() => {
  return dealCategory(currentSession.value?.category);
});

const currentMenuList = ref<('event' | 'visual')[]>(['event']);
const menus = [
  {
    key: 'event',
    title: t('journalConfig.Event'),
    icon: UnorderedListOutlined,
  },
];

const isCurrentMenuItem = (key: 'event' | 'visual') =>
  currentMenuList.value.includes(key);

const exportFileName = computed(() => {
  if (currentSession.value) {
    return `${
      currentSession.value.session_id_resolved
    }_${currentSession.value.begin_time_resolved
      .split('.')[0]
      .split(':')
      .join('-')}`;
  }

  return 'session';
});

const sessionColumns = getSessionColumns();

watch(
  () => sessions.value,
  () => {
    journalStore.setSessions(sessions.value);
    allLocations.value = io.getAllLocations();
    SourceAndDestNameMap.value = dealsessionsToMap(allLocations.value);
  },
  {
    deep: true,
  },
);

watch(
  () => mdSession.value?.end_time,
  () => {
    getMdSessions();
  },
);

watch(
  () => currentSession.value,

  (newSession) => {
    if (!newSession) return;
    if (currentSessionId.value !== newSession.location_uid) {
      currentSessionId.value = newSession.location_uid;
    } else if (
      beginTime.value === newSession.begin_time &&
      newSession.status === SessionStatusEnum.Finished
    ) {
      currentTime.value = newSession.begin_time;
    }
    const { begin_time, end_time } = newSession;
    beginTime.value = begin_time;
    endTime.value = end_time ?? 0n;
  },
  {
    deep: true,
  },
);

const onUpdateCurrentTime = (value: bigint) => {
  currentTime.value = value;
};

const dealsessionsToMap = (sessions: Record<string, LocationRseolved>) => {
  const SourceAndDestNameMap: Record<string, LocationRseolved> = {};
  Object.values(sessions).forEach((item) => {
    SourceAndDestNameMap[item.uid] = item;
  });
  return ransformObject(SourceAndDestNameMap);
};
const ransformObject = (obj: Record<string, LocationRseolved>) => {
  const output: Record<string, string> = {};

  for (const key in obj) {
    // eslint-disable-next-line no-prototype-builtins
    if (obj.hasOwnProperty(key)) {
      const item = obj[key];
      output[key] = `${item.category}/${item.group}/${item.name}/${item.mode}`;
    }
  }
  output['0'] = 'public';
  output['1'] = 'sync';

  return output;
};

const getSessions = () => {
  return currentLocation
    ? sessionStore.getSessionsForLocation(currentLocation)
    : sessionStore.getAllSessions();
};

const getMdSessions = () => {
  const sessions = sessionStore.getAllSessions();
  if (!sessions) {
    mdSession.value = null;
    return null;
  }
  const mdSessions = sessions.filter((item) => item.name === 'sim');
  const mdRuningSessions = mdSessions.filter(
    (item) => item.end_time === BigInt(0),
  );
  if (mdRuningSessions.length) {
    mdSession.value = mdRuningSessions[mdRuningSessions.length - 1];
    return mdRuningSessions[mdRuningSessions.length - 1];
  }
  mdSession.value = mdSessions[0];
  return mdSession.value;
};

const loadSessions = (gotSessions?: KungfuApi.Session[]) => {
  const currentSessions = gotSessions ?? getSessions();
  if (currentSessions?.length) {
    sessionsMap.value = dealSessionsToMap(currentSessions.reverse());
    nextTick(() => {
      if (!gotSessions && sessions.value.length) {
        const { index, begin_time } = sessions.value[0];

        currentSessionKey.value = `${begin_time}`;
        currentSessionId.value = index;
      }
    });
  }
};

const startCheckSessionsStatus = () => {
  setTimerPromiseTask(async () => {
    let hasNewSession = false;
    const currentSessions = getSessions();
    const sessionsStatusMap = currentSessions?.reduce((map, session) => {
      const key = `${session.begin_time}`;
      map[key] = session.end_time === 0n ? false : session.end_time;
      if (!(key in sessionsMap.value)) {
        hasNewSession = true;
      }
      return map;
    }, {});
    if (hasNewSession) {
      loadSessions(currentSessions);
      getMdSessions();
      return;
    }

    toRaw(runningSessions.value).forEach((session) => {
      const currentKey = `${session.begin_time}`;
      const currentEndTime = sessionsStatusMap?.[currentKey];

      if (currentEndTime) {
        sessionsMap.value[currentKey].end_time = currentEndTime;
        sessionsMap.value[currentKey].end_time_resolved = dealKfTime(
          getAbs<bigint>(currentEndTime),
        );
        sessionsMap.value[currentKey].status = SessionStatusEnum.Finished;
      }
    });
  }, 1000);
};

onMounted(() => {
  loadSessions();
  removeLoadingMask();
  startCheckSessionsStatus();
  getMdSessions();

  window.addEventListener('resize', () => {
    app?.proxy &&
      app?.proxy.$globalBus.next({
        tag: 'resize',
      } as KfEvent.ResizeEvent);
  });

});

const handleSelectSession = ({ row }) => {
  currentSessionId.value = row.index;
  currentSessionKey.value = row.begin_time + '';
};

const onExportJournalData = (
  exportData: (fileName: string, exportData: KungfuApi.FrameResolved[]) => void,
) => {
  exportData(exportFileName.value, eventDashBoard.value.frameDataList);
};

const dealRowClassName = (row) => {
  return `${row.begin_time}` === currentSessionKey.value
    ? 'kf-current-table-select'
    : '';
};
</script>

<style lang="less">
@import '@kungfu-trader/kungfu-app/src/renderer/assets/less/base.less';
@import '@kungfu-trader/kungfu-app/src/renderer/assets/less/public.less';
@import '@kungfu-trader/kungfu-app/src/renderer/assets/less/coverAnt.less';
@import '@kungfu-trader/kungfu-app/src/renderer/assets/less/variables.less';

#app {
  width: 100%;
  height: 100%;
  font-family: Consolas, Monaco, Lucida Console, Liberation Mono,
    DejaVu Sans Mono, Bitstream Vera Sans Mono, Courier New, monospace;
  -webkit-font-smoothing: antialiased;
  -moz-osx-font-smoothing: grayscale;
  text-align: center;
  .ant-tooltip {
    color: #6d0d0d !important;
    position: absolute !important;
    top: 100px !important;
    left: 100px !important;
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

      .kf-journal-session__wrap {
        flex: 0 0 300px;
        height: 300px;
        width: 60%;
        margin: auto;
        padding: 32px 0;
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
          max-width: 560px;
          flex: 0 1 560px;

          .ant-tooltip {
            // 你的样式
            color: #6d0d0d !important;
            position: absolute !important;
            top: 100px !important;
            left: 100px !important;
          }
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

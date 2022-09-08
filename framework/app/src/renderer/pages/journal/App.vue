<template>
  <a-layout>
    <div class="kf-journal-view__wrap">
      <div class="kf-journal-session__wrap">
        <KfTradingDataTable
          v-model:selected-key="currentSessionKey"
          :selectable="true"
          :data-source="sessions"
          :columns="sessionColumns"
          key-field="begin_time"
          :resizable="false"
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
            <template v-else>
              <span>
                {{ item[column.dataIndex as keyof KungfuApi.SessionResolved] }}
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
          v-model:time-range="currentTimeRange"
          :limit-time-range="limitTimeRange"
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
            :current-session-id="currentSessionId"
            :current-session-closed="currentSession?.is_closed || true"
            :current-time-range="currentTimeRange"
            :session-location-map="sessionLocationMap"
          />
          <OrdersDashboard v-show="isCurrentMenuItem('order')" />
        </div>
      </div>
    </div>
  </a-layout>
</template>

<script setup lang="ts">
import { onMounted, ref, computed, toRaw } from 'vue';
import { assemble, dealKfTime } from '@kungfu-trader/kungfu-js-api/kungfu';
import { getSessionColumns } from './config';
import { removeLoadingMask } from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/uiUtils';
import { getCurrentLocation, dealCategory } from './utils';
import {
  KfCategoryEnum,
  KfCategoryTypes,
} from '@kungfu-trader/kungfu-js-api/typings/enums';
import { getProcessIdByKfLocation } from '@kungfu-trader/kungfu-js-api/utils/busiUtils';
import {
  UnorderedListOutlined,
  LineChartOutlined,
} from '@ant-design/icons-vue';
import KfTradingDataTable from '@kungfu-trader/kungfu-app/src/renderer/components/public/KfTradingDataTable.vue';
import TimeSlider from './components/TimeSlider.vue';
import ExportJournal from './components/ExportJournal.vue';
import EventsDashBoard from './components/EventsDashBoard.vue';
import OrdersDashboard from './components/OrdersDashboard.vue';

const currentLocation = getCurrentLocation();
const eventDashBoard = ref();

const sessions = ref<KungfuApi.SessionResolved[]>([]);
const sessionLocationMap = computed(() => {
  return sessions.value.reduce((pre, session) => {
    pre[session.location_uid] = {
      category: session.category,
      group: session.group,
      name: session.name,
      mode: 'live',
    };
    return pre;
  }, {} as Record<number, KungfuApi.KfLocation>);
});
const sessionsMap = computed(() => {
  return sessions.value.reduce((pre, session) => {
    pre[`${session.begin_time}`] = toRaw(session);
    return pre;
  }, {} as Record<string, KungfuApi.SessionResolved>);
});

const currentSessionKey = ref('');
const currentSessionId = ref(-1);
const currentTimeRange = ref<[bigint, bigint]>([0n, 10000n]);
const limitTimeRange = ref<[bigint, bigint]>([0n, 10000n]);

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

const currentMenuList = ref<('event' | 'order')[]>(['event']);
const menus = [
  {
    key: 'event',
    title: 'Event',
    icon: UnorderedListOutlined,
  },
  {
    key: 'order',
    title: 'Order',
    icon: LineChartOutlined,
  },
];

const isCurrentMenuItem = (key: 'event' | 'order') =>
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

const getAbs = <T extends number | bigint>(num: T): T =>
  num < 0 ? (-num as T) : num;

const loadSessions = () => {
  setTimeout(() => {
    let currentSessions = assemble.get_sessions();

    if (currentSessions.length) {
      sessions.value = currentSessions
        .map((item, index) => {
          item.category = KfCategoryEnum[
            item.category as KfCategoryEnum
          ] as KfCategoryTypes;
          return {
            index,
            ...item,
            session_id_resolved: getProcessIdByKfLocation(item),
            begin_time_resolved: dealKfTime(getAbs<bigint>(item.begin_time)),
            end_time_resolved: dealKfTime(getAbs<bigint>(item.end_time)),
            is_closed: item.end_time != 0n,
          };
        })
        .filter((item) => {
          if (currentLocation) {
            return (
              currentLocation.location_uid === item.location_uid &&
              !!assemble.get_reader(item.index)
            );
          } else {
            return !!assemble.get_reader(item.index);
          }
        })
        .reverse();

      if (sessions.value.length) {
        const { index, begin_time } = sessions.value[0];

        currentSessionKey.value = `${begin_time}`;
        currentSessionId.value = index;
        setTimeRange(sessions.value[0]);
      }
    }
  });
};

onMounted(() => {
  loadSessions();
  removeLoadingMask();
});

const handleSelectSession = ({ row }) => {
  currentSessionId.value = row.index;
  setTimeRange(row);
};

const onExportJournalData = (
  exportData: (fileName: string, exportData: KungfuApi.FrameResolved[]) => void,
) => {
  exportData(exportFileName.value, eventDashBoard.value.frameDataList);
};

function setTimeRange(session: KungfuApi.Session) {
  const { begin_time, end_time } = session;

  currentTimeRange.value = [
    begin_time,
    end_time ? end_time : BigInt(new Date().getTime() * 1000),
  ];

  limitTimeRange.value = [
    begin_time,
    end_time ? end_time : BigInt(new Date().getTime() * 1000),
  ];
}
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
        flex: 0 300px;
        height: 300px;
        width: 60%;
        margin: auto;
        padding: 32px 0;
        box-sizing: border-box;
      }

      .kf-journal-control-bar {
        flex: 0 40px;
        height: 40px;
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
        }
      }

      .kf-journal-menu__wrap {
        width: 100%;
        flex: auto;

        display: flex;

        .kf-journal-menu-tab {
          flex: 0 120px;
          width: 120px;
          margin-right: 2px;

          li {
            width: 100%;
          }
        }

        .kf-journal-menu-content {
          flex: auto;
          height: 100%;
        }
      }
    }
  }
}
</style>

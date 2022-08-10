<template>
  <a-layout>
    <div class="kf-journal-view__wrap">
      <div class="kf-journal-session__wrap">
        <KfTradingDataTable
          :data-source="sessions"
          :columns="sessionColumns"
          key-field="begin_time"
          :resizable="false"
          :selectable="true"
          :default-selected-index="0"
          @click-cell="handleSelectSession"
        ></KfTradingDataTable>
      </div>
      <div class="kf-journal-control-bar">
        <div class="kf-journal-control-filters"></div>
        <a-button>
          {{ t('journalConfig.export') }}
        </a-button>
      </div>
      <div class="kf-journal-frame__wrap">
        <KfTradingDataTable
          :data-source="frameDataList"
          :columns="frameColumns"
          key-field="genTime"
          :resizable="false"
          :selectable="true"
          @click-cell="handleOpenFrameDetail"
        ></KfTradingDataTable>
      </div>
    </div>
  </a-layout>
  <a-drawer v-model:visible="visible" title="Event Frame" placement="right">
    <div class="frame-detail-drawer__wrap">
      <template v-if="currentRowDataResolved">
        <a-list size="normal" bordered :data-source="currentRowDataResolved">
          <template #renderItem="{ item }">
            <a-list-item>
              <span class="frame-detail-datalist-key">{{ item.key }}</span>
              <span class="frame-detail-datalist-value">{{ item.value }}</span>
            </a-list-item>
          </template>
        </a-list>

        <a-card title="Frame Data" style="margin-top: 35px">
          <a-tree
            :show-line="true"
            :show-icon="true"
            :tree-data="currentRowData.data_resolved"
            :selectable="false"
            default-expand-all
          >
            <template #switcherIcon="{ defaultIcon }">
              <component :is="defaultIcon" />
            </template>
          </a-tree>
        </a-card>
      </template>
      <template v-else>
        <a-empty :image="simpleImage"></a-empty>
      </template>
    </div>
  </a-drawer>
</template>

<script setup lang="ts">
import { onMounted, ref, computed } from 'vue';
import { assemble, dealKfTime } from '@kungfu-trader/kungfu-js-api/kungfu';
import { getSessionColumns, getFrameColumns } from './config';
import { removeLoadingMask } from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/uiUtils';
import KfTradingDataTable from '@kungfu-trader/kungfu-app/src/renderer/components/public/KfTradingDataTable.vue';
import { dealFrame, getCurrentLocation } from './utils';
import { message, Empty } from 'ant-design-vue';
import VueI18n from '@kungfu-trader/kungfu-js-api/language';
import {
  KfCategoryEnum,
  KfCategoryTypes,
} from '@kungfu-trader/kungfu-js-api/typings/enums';

const { t } = VueI18n.global;
const currentLocation = getCurrentLocation();

const sessions = ref<KungfuApi.SessionResolved[]>([]);
const sessionMap = computed(() => {
  return sessions.value.reduce((pre, session) => {
    pre[session.location_uid] = {
      category: KfCategoryEnum[
        session.category as KfCategoryEnum
      ] as KfCategoryTypes,
      group: session.group,
      name: session.name,
      mode: 'live',
    };
    return pre;
  }, {} as Record<number, KungfuApi.KfLocation>);
});

const currentSessionIndex = ref<number>(0);

const limitReadCount = ref(10000);
const frameDataList = ref<KungfuApi.FrameResolved[]>([]);

const visible = ref(false);
const currentRowData = ref<KungfuApi.FrameResolved | Record<string, never>>({});
const currentRowDataResolved = computed(() => {
  if (currentRowData.value) {
    return Object.keys(currentRowData.value)
      .map((item) => {
        if (item.includes('_') || item === 'data') return null;

        return {
          key: item as unknown as keyof KungfuApi.FrameResolved,
          value: `${currentRowData.value[item]}`,
        };
      })
      .filter((item) => !!item);
  }

  return null;
});

const simpleImage = Empty.PRESENTED_IMAGE_SIMPLE;

const sessionColumns = getSessionColumns();
const frameColumns = getFrameColumns();

onMounted(() => {
  removeLoadingMask();
});

const getAbs = <T extends number | bigint>(num: T): T =>
  num < 0 ? (-num as T) : num;

const loadSessions = () => {
  let currentSessions = assemble.get_sessions();
  if (currentLocation) {
    currentSessions = currentSessions.filter(
      (item) => item.location_uid === currentLocation.location_uid,
    );
  }

  if (currentSessions.length) {
    sessions.value = currentSessions
      .map((item, index) => {
        return {
          ...item,
          index,
          begin_time_resolved: dealKfTime(getAbs<bigint>(item.begin_time)),
          end_time_resolved: dealKfTime(getAbs<bigint>(item.end_time)),
          is_closed: !!item.end_time,
        };
      })
      .reverse();

    currentSessionIndex.value = sessions.value[0].index;
    loadFrameData(currentSessionIndex.value);
  }
};

const loadFrameData = (sessionId: number) => {
  const hideloading = message.loading(t('journalConfig.loading_journal'));
  console.log('sessionId', sessionId);

  return new Promise<KungfuApi.FrameResolved[]>((resolve, reject) => {
    try {
      const journalReader = assemble.get_reader(sessionId);
      const currentSession = sessions.value.filter(
        (item) => item.index === sessionId,
      )[0];

      const targetCount = Math.min(
        currentSession.frame_count,
        limitReadCount.value,
      );

      frameDataList.value = [];

      const frames: Record<string, KungfuApi.FrameResolved> = {};
      let count = 0;
      journalReader.run((frame) => {
        const curFrameData: KungfuApi.Frame = {
          dataLength: frame.dataLength(),
          genTime: frame.genTime(),
          triggerTime: frame.triggerTime(),
          msgType: frame.msgType(),
          stringMsgType: frame.stringMsgType(),
          source: frame.source(),
          dest: frame.dest(),
          data: frame.data(),
        };

        if (!(`${curFrameData.genTime}` in frames)) {
          frames[`${curFrameData.genTime}`] = dealFrame(
            curFrameData,
            sessionMap.value,
          );
        }

        if (++count >= targetCount) {
          resolve(Object.values(frames));
        }
      }, targetCount);
    } catch (error) {
      reject(error);
    }
  }).then((res) => {
    frameDataList.value = res;
    hideloading();
  });
};

const handleSelectSession = ({ row }) => {
  loadFrameData(row.index);
};

const handleOpenFrameDetail = ({ row }) => {
  visible.value = true;
  currentRowData.value = row;
};

loadSessions();
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

      .kf-journal-session__wrap {
        height: 300px;
      }

      .kf-journal-control-bar {
        height: 40px;
        background-color: #1d1d1d;
        padding: 5px 20px;
        margin-bottom: 2px;
        display: flex;
        justify-content: space-between;
        align-items: center;
      }

      .kf-journal-frame__wrap {
        height: calc(100% - 350px);
      }
    }
  }
}

.frame-detail-drawer__wrap {
  height: 100%;
  width: 100%;
  padding: 30px 20px;

  .ant-tree-switcher-noop {
    display: none;
  }
}
</style>

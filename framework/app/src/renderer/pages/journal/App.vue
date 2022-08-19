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
        ></KfTradingDataTable>
      </div>
      <div class="kf-journal-control-bar">
        <div class="kf-journal-bar-title">
          {{ `session: ${currentSessionTitle}` }}
        </div>
        <div class="kf-journal-control-filters"></div>
        <ExportJournal
          :export-data="frameDataList"
          :file-name="exportFileName"
        />
      </div>
      <div class="kf-journal-filters-bar">
        <div class="kf-journal-bar-title">
          {{ `${$t('journalConfig.filters')}: ` }}
        </div>
        <FrameFilters
          ref="frameFilter"
          @apply-filters="onFiltersApply"
        ></FrameFilters>
      </div>
      <div class="kf-journal-frame__wrap">
        <KfTradingDataTable
          v-model:selected-key="currentFramesKey"
          :selectable="true"
          :data-source="frameDataListResolved"
          :columns="frameColumns"
          key-field="genTime"
          :resizable="false"
          @click-cell="handleOpenFrameDetail"
        ></KfTradingDataTable>
      </div>
      <a-spin
        :spinning="loadingJournal"
        :tip="t('journalConfig.loading_journal')"
      />
    </div>
  </a-layout>
  <a-drawer
    v-model:visible="visible"
    title="Event Frame"
    placement="right"
    :force-render="true"
  >
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
            :tree-data="framesMap[currentFramesKey].data_resolved"
            :selectable="false"
            default-expand-all
          ></a-tree>
        </a-card>
      </template>
      <template v-else>
        <a-empty :image="simpleImage"></a-empty>
      </template>
    </div>
  </a-drawer>
</template>

<script setup lang="ts">
import { onMounted, ref, computed, toRaw } from 'vue';
import { assemble, dealKfTime } from '@kungfu-trader/kungfu-js-api/kungfu';
import { getSessionColumns, getFrameColumns } from './config';
import { removeLoadingMask } from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/uiUtils';
import { dealFrame, getCurrentLocation } from './utils';
import { Empty } from 'ant-design-vue';
import VueI18n from '@kungfu-trader/kungfu-js-api/language';
import {
  KfCategoryEnum,
  KfCategoryTypes,
} from '@kungfu-trader/kungfu-js-api/typings/enums';
import { getIdByKfLocation } from '@kungfu-trader/kungfu-js-api/utils/busiUtils';
import { createFiltersEnumMap, FiltersEnum } from './utils/filterUtils';
import KfTradingDataTable from '@kungfu-trader/kungfu-app/src/renderer/components/public/KfTradingDataTable.vue';
import ExportJournal from './components/ExportJournal.vue';
import FrameFilters from './components/FrameFilters.vue';

const { t } = VueI18n.global;
const currentLocation = getCurrentLocation();
const frameFilter = ref();

const loadingJournal = ref(false);

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
const currentFramesKey = ref('');

const currentSessionTitle = computed(() => {
  if (currentSessionKey.value && Object.keys(sessionsMap.value).length) {
    const currentSession = sessionsMap.value[currentSessionKey.value];
    if (currentSession) {
      return `${currentSession.name_resolved}   time: ${currentSession.begin_time_resolved} - ${currentSession.end_time_resolved}`;
    }

    return '';
  }

  return '';
});

const exportFileName = computed(() => {
  if (currentSessionKey.value && Object.keys(sessionsMap.value).length) {
    const currentSession = sessionsMap.value[currentSessionKey.value];
    if (currentSession) {
      return `${
        currentSession.name_resolved
      }_${currentSession.begin_time_resolved
        .split('.')[0]
        .split(':')
        .join('-')}`;
    }
  }

  return 'session';
});

const frameDataList = ref<KungfuApi.FrameResolved[]>([]);
const frameFiltersReg = ref(createFiltersEnumMap(''));
const frameDataListResolved = computed(() => {
  return frameDataList.value.filter((item) => {
    return Object.keys(frameFiltersReg.value).every((filterKey) => {
      if (frameFiltersReg.value[filterKey]) {
        console.log(frameFiltersReg.value[filterKey]);
        const curReg = new RegExp(`^${frameFiltersReg.value[filterKey]}$`);
        switch (filterKey) {
          case FiltersEnum.DEST:
            return curReg.test(item.dest + '');
          case FiltersEnum.SOURCE:
            return curReg.test(item.source + '');
          case FiltersEnum.MSG_TYPE:
            return curReg.test(item.msgType + '');
        }

        return true;
      } else {
        return true;
      }
    });
  });
});
const framesMap = ref<Record<string, KungfuApi.FrameResolved>>({});

const visible = ref(false);
const currentRowDataResolved = computed(() => {
  const currentRowData = framesMap.value[currentFramesKey.value];
  if (currentRowData) {
    return Object.keys(currentRowData)
      .map((item) => {
        if (item.includes('_') || item === 'data') return null;

        return {
          key: item as unknown as keyof KungfuApi.FrameResolved,
          value: `${currentRowData[item]}`,
        };
      })
      .filter((item) => !!item);
  }

  return null;
});

const simpleImage = Empty.PRESENTED_IMAGE_SIMPLE;

const sessionColumns = getSessionColumns();
const frameColumns = getFrameColumns();

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
            name_resolved: getIdByKfLocation(item),
            begin_time_resolved: dealKfTime(getAbs<bigint>(item.begin_time)),
            end_time_resolved: dealKfTime(getAbs<bigint>(item.end_time)),
            is_closed: item.end_time < 0,
          };
        })
        .filter((item) => {
          if (currentLocation) {
            return currentLocation.location_uid === item.location_uid;
          } else {
            return true;
          }
        })
        .reverse();

      if (sessions.value.length) {
        const { index, begin_time } = sessions.value[0];

        currentSessionKey.value = `${begin_time}`;
        loadFrameData(index);
      }
    }
  });
};

const loadFrameData = (sessionId: number) => {
  loadingJournal.value = true;
  console.log('sessionId', sessionId);

  const journalReader = assemble.get_reader(sessionId);

  framesMap.value = {};

  return new Promise<KungfuApi.FrameResolved[]>((resolve, _) => {
    const runner = () => {
      setTimeout(() => {
        const isUnfinish = journalReader.next();

        if (isUnfinish) {
          const frame = journalReader.currentFrame();

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

          if (!(`${curFrameData.genTime}` in framesMap.value)) {
            const curFrameDataResolved = dealFrame(
              curFrameData,
              sessionLocationMap.value,
            );

            framesMap.value[`${curFrameData.genTime}`] = curFrameDataResolved;

            frameFilter.value.addOption(FiltersEnum.DEST, {
              label: curFrameDataResolved.dest_resolved,
              value: curFrameDataResolved.dest + '',
            });
            frameFilter.value.addOption(FiltersEnum.SOURCE, {
              label: curFrameDataResolved.source_resolved,
              value: curFrameDataResolved.source + '',
            });
            frameFilter.value.addOption(FiltersEnum.MSG_TYPE, {
              label: curFrameDataResolved.stringMsgType,
              value: curFrameDataResolved.msgType + '',
            });
          }

          runner();
        } else {
          resolve(Object.values(toRaw(framesMap.value)));
        }
      });
    };

    runner();
  }).then((res) => {
    console.log(res);
    frameDataList.value = res;
    loadingJournal.value = false;
  });

  // return new Promise<KungfuApi.FrameResolved[]>((resolve, reject) => {
  //   try {
  //     const currentSession = sessions.value.filter(
  //       (item) => item.index === sessionId,
  //     )[0];
  //     const targetCount = Math.min(currentSession.frame_count, 10000);
  //     frameDataList.value = [];
  //     const frames: Record<string, KungfuApi.FrameResolved> = {};
  //     let count = 0;
  //     journalReader.run((frame) => {
  //       const curFrameData: KungfuApi.Frame = {
  //         dataLength: frame.dataLength(),
  //         genTime: frame.genTime(),
  //         triggerTime: frame.triggerTime(),
  //         msgType: frame.msgType(),
  //         stringMsgType: frame.stringMsgType(),
  //         source: frame.source(),
  //         dest: frame.dest(),
  //         data: frame.data(),
  //       };
  //       if (!(`${curFrameData.genTime}` in frames)) {
  //         frames[`${curFrameData.genTime}`] = dealFrame(
  //           curFrameData,
  //           sessionMap.value,
  //         );
  //       }
  //       if (++count >= targetCount) {
  //         resolve(Object.values(frames));
  //       }
  //     }, targetCount);
  //   } catch (error) {
  //     reject(error);
  //   }
  // }).then((res) => {
  //   frameDataList.value = res;
  //   hideloading();
  // });
};

onMounted(() => {
  removeLoadingMask();
  loadSessions();
});

const handleSelectSession = ({ row }) => {
  loadFrameData(row.index);
};

const handleOpenFrameDetail = () => {
  visible.value = true;
};

const onFiltersApply = (filtersFormState: Record<FiltersEnum, string>) => {
  Object.keys(filtersFormState).forEach((key) => {
    frameFiltersReg.value[key] = filtersFormState[key];
  });
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

      .kf-journal-bar {
        flex: 0 40px;
        height: 40px;
        background-color: #1d1d1d;
        padding: 5px 20px;
        margin-bottom: 2px;
        display: flex;
        align-items: center;

        .kf-journal-bar-title {
          font-size: 14px;
          margin-right: 16px;
        }
      }

      .kf-journal-control-bar {
        .kf-journal-bar();
        justify-content: space-between;
      }

      .kf-journal-filters-bar {
        .kf-journal-bar();
        justify-content: flex-start;
      }

      .kf-journal-frame__wrap {
        flex: auto;
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

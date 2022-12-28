<template>
  <div class="kf-journal-events__wrap">
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
        :selectable="true"
        :data-source="frameDataListResolved"
        :columns="frameColumns"
        key-field="id"
        :resizable="false"
        :custom-row-class="dealRowClassName"
        @click-cell="handleOpenFrameDetail"
      >
        <template
          #default="{
            item,
            column,
          }: {
            item: KungfuApi.FrameResolved,
            column: KfTradingDataTableHeaderConfig,
          }"
        >
          <template v-if="column.dataIndex === 'stringMsgType'">
            <a-tag
              :style="{
                color: item.msgTypeResolved.color || 'rgb(158, 158, 158)',
                backgroundColor: dealTagBackgroudColor(
                  item.msgTypeResolved.color || 'rgb(158, 158, 158)',
                ),
                border: `1px solid ${
                  item.msgTypeResolved.color || 'rgb(158, 158, 158)'
                }`,
              }"
            >
              {{ item.stringMsgType }}
            </a-tag>
          </template>
          <template v-else>
            <span>
              {{ item[column.dataIndex as keyof KungfuApi.FrameResolved] }}
            </span>
          </template>
        </template>
      </KfTradingDataTable>
    </div>
    <a-spin
      class="kf-journal-spin"
      :spinning="loadingJournal"
      :tip="$t('journalConfig.loading_journal')"
    />
    <a-drawer
      v-model:visible="visible"
      title="Event Frame"
      placement="right"
      :force-render="true"
    >
      <template v-if="currentRowDataResolved">
        <a-list size="normal" bordered :data-source="currentRowDataResolved">
          <template #renderItem="{ item }">
            <a-list-item>
              <span class="frame-detail-datalist-key">{{ item.key }}</span>
              <span class="frame-detail-datalist-value">
                {{ item.value }}
              </span>
            </a-list-item>
          </template>
        </a-list>

        <a-card title="Frame Data" style="margin: 35px 0">
          <a-tree
            :show-line="true"
            :show-icon="true"
            :tree-data="framesMap[currentFramesId].dataResolved"
            :selectable="false"
            default-expand-all
          ></a-tree>
        </a-card>
      </template>
      <template v-else>
        <a-empty :image="simpleImage"></a-empty>
      </template>
    </a-drawer>
  </div>
</template>

<script lang="ts" setup>
import { ref, computed, watch, shallowRef } from 'vue';
import { Empty } from 'ant-design-vue';
import { assemble } from '@kungfu-trader/kungfu-js-api/kungfu';
import { getFrameColumns } from '../config';
import { dealFrame } from '../utils';
import { createFiltersEnumMap, FiltersEnum } from '../utils/filterUtils';
import KfTradingDataTable from '@kungfu-trader/kungfu-app/src/renderer/components/public/KfTradingDataTable.vue';
import FrameFilters from './FrameFilters.vue';
import { useJournalStore } from '../store/journalStore';

const props = withDefaults(
  defineProps<{
    currentSession: KungfuApi.SessionResolved | null;
    currentTimeRangeData: {
      range: [bigint, bigint];
      reload: boolean;
    };
  }>(),
  {},
);

const journalStore = useJournalStore();
const frameColumns = getFrameColumns();
const simpleImage = Empty.PRESENTED_IMAGE_SIMPLE;
const loadingJournal = ref(false);

const currentFramesId = ref<number>(-1);
const frameFilter = ref();

const frameDataList = shallowRef<KungfuApi.FrameResolved[]>([]);
const frameFiltersReg = ref(createFiltersEnumMap(/.*/));
const frameDataListResolved = computed(() => {
  return frameDataList.value.filter((item) => {
    return Object.keys(frameFiltersReg.value).every((filterKey) => {
      if (frameFiltersReg.value[filterKey]) {
        const curReg = frameFiltersReg.value[filterKey as FiltersEnum];
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

const framesMap = shallowRef<Record<string, KungfuApi.FrameResolved>>({});

const visible = ref(false);
const excludeRowData = ['data', 'sourceToDest'];
const currentRowDataResolved = computed(() => {
  const currentRowData = framesMap.value[currentFramesId.value];
  if (currentRowData) {
    return Object.keys(currentRowData)
      .map((item) => {
        if (
          item.indexOf('Resolved') !== -1 ||
          excludeRowData.indexOf(item) !== -1
        )
          return null;

        const key = currentRowData[`${item}Resolved`]
          ? `${item}Resolved`
          : item;

        const value =
          item === 'msgType'
            ? currentRowData.msgType
            : `${currentRowData[key]}`;

        return {
          key: item as unknown as keyof KungfuApi.FrameResolved,
          value,
        };
      })
      .filter((item) => !!item);
  }

  return null;
});

watch(
  () => props.currentSession,
  (newSession, oldSession) => {
    if (newSession && newSession !== oldSession) {
      loadFrameData(newSession, newSession.begin_time, newSession.end_time);
    }
  },
);

watch(
  () => props.currentTimeRangeData,
  (newRangeData) => {
    if (props.currentSession) {
      if (newRangeData.range[0] && newRangeData.range[1]) {
        loadFrameData(
          props.currentSession,
          newRangeData.range[0],
          newRangeData.range[1],
          !newRangeData.reload,
        );
      }
    }
  },
  { deep: true },
);

let journalReader: KungfuApi.AssembleReader | null = null;
let lastReaderArgs = {
  sessionId: 0,
  startTime: 0n,
  endTime: 0n,
};
const LIMIT_COUNT = 100000;

const checkReaderArgs = (args: {
  sessionId: number;
  startTime: bigint;
  endTime: bigint;
}) => {
  if (
    args.sessionId === lastReaderArgs.sessionId &&
    args.startTime === lastReaderArgs.startTime &&
    args.endTime === lastReaderArgs.endTime
  ) {
    return false;
  } else {
    lastReaderArgs = args;
    return true;
  }
};

const loadFrameData = (
  session: KungfuApi.SessionResolved,
  startTime: bigint,
  endTime: bigint,
  checking = false,
) => {
  const sessionId = session.index;

  if (!checkReaderArgs({ sessionId, startTime, endTime })) return;

  if (!checking) {
    loadingJournal.value = true;
    if (!session.is_closed) {
      journalReader = assemble.get_reader(sessionId, startTime);
    } else {
      journalReader = assemble.get_reader(sessionId, startTime, endTime);
    }
    framesMap.value = {};
  }

  let total = 0;
  const curFramesMap = {};

  return new Promise<KungfuApi.FrameResolved[]>((resolve, _) => {
    const runner = () => {
      setTimeout(() => {
        if (!journalReader) return resolve([]);
        let count = 0;
        journalReader.run((frame) => {
          if (frame) {
            const curFrameData: KungfuApi.Frame = {
              id: total,
              dataLength: frame.dataLength(),
              genTime: frame.genTime(),
              triggerTime: frame.triggerTime(),
              msgType: frame.msgType(),
              stringMsgType: frame.stringMsgType(),
              source: frame.source(),
              dest: frame.dest(),
              data: frame.data(),
              destName: frame.destName(),
              sourceName: frame.sourceName(),
            };

            const curFrameDataResolved = dealFrame(curFrameData);

            curFramesMap[curFrameDataResolved.id] = curFrameDataResolved;
            framesMap.value[curFrameDataResolved.id] = curFrameDataResolved;

            frameFilter.value?.addOption(FiltersEnum.DEST, [
              {
                label: curFrameDataResolved.destName,
                value: curFrameDataResolved.dest + '',
              },
            ]);
            frameFilter.value?.addOption(FiltersEnum.SOURCE, [
              {
                label: curFrameDataResolved.sourceName,
                value: curFrameDataResolved.source + '',
              },
            ]);
            frameFilter.value?.addOption(FiltersEnum.MSG_TYPE, [
              {
                label: curFrameDataResolved.stringMsgType,
                value: curFrameDataResolved.msgType + '',
              },
            ]);

            ++total;
            ++count;
          }
        }, 10);

        if (count < 10 || total >= LIMIT_COUNT) {
          resolve(Object.values(curFramesMap));
        } else {
          runner();
        }
      });
    };

    runner();
  }).then((res) => {
    console.log(res);
    console.log(total);

    if (checking) {
      frameDataList.value.push(...res);
      journalStore.setCurrentSessionFrames(res, false);
    } else {
      frameDataList.value = res;
      currentFramesId.value = frameDataList.value[0]?.id;
      journalStore.setCurrentSessionFrames(res, true);
    }

    loadingJournal.value = false;
  });
};

const handleOpenFrameDetail = ({ row }) => {
  currentFramesId.value = row.id;
  visible.value = true;
};

const onFiltersApply = (filtersFormState: Record<FiltersEnum, string[]>) => {
  Object.keys(filtersFormState).forEach((key) => {
    frameFiltersReg.value[key] = new RegExp(
      `^${
        filtersFormState[key].reduce<string>(
          (str: string, cur: string) => str + `(${cur})?`,
          '',
        ) || '.*'
      }$`,
    );
  });
};

const dealTagBackgroudColor = (color: string) => {
  if (!color) return '';
  if (color.indexOf('rgb') !== -1 && color.indexOf('rgba') === -1) {
    const rgba = color.substring(4, color.length - 1) + ', 0.3';
    return `rgba(${rgba})`;
  }
  return color;
};

const dealRowClassName = (row) => {
  return row.id === currentFramesId.value ? 'kf-current-table-select' : '';
};

defineExpose({
  frameDataList,
});
</script>

<style lang="less">
.kf-journal-events__wrap {
  width: 100%;
  height: 100%;
  display: flex;
  flex-direction: column;

  .kf-journal-spin {
    .ant-spin-text {
      margin-left: 8px;
    }
  }

  .kf-journal-filters-bar {
    flex: 0 40px;
    height: 40px;
    background-color: #1d1d1d;
    padding: 5px 20px;
    margin-bottom: 2px;
    display: flex;
    align-items: center;
    justify-content: flex-start;

    .kf-journal-bar-title {
      font-size: 14px;
      margin-right: 16px;
    }
  }

  .kf-journal-frame__wrap {
    flex: auto;
  }
}

.ant-drawer-body {
  margin-bottom: 8px;

  .ant-tree-switcher-noop {
    display: none;
  }
}
</style>

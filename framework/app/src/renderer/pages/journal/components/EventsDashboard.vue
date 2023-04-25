<template>
  <div class="kf-journal-events__wrap">
    <div class="kf-journal-filters-bar">
      <!-- <div class="kf-journal-bar-title">
        {{ `${$t('journalConfig.filters')}: ` }}
      </div> -->
      <FrameFilters
        ref="frameFilter"
        :location-map="locationMap"
        :current-location="currentLocation"
        :current-time-range="currentTimeRangeData.range"
        @apply-filters="onFiltersApply"
      ></FrameFilters>
    </div>
    <div class="kf-journal-frame__wrap">
      <KfTradingDataTable
        :data-source="frameDataListResolved"
        :columns="frameColumns"
        key-field="id"
        :resizable="false"
        :custom-row-class="dealRowClassName"
        @click-cell="handleOpenFrameDetail"
        @click-row="handleOpenFrameDetail"
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
import { longfist, tracer } from '@kungfu-trader/kungfu-js-api/kungfu';
import { getFrameColumns } from '../config';
import { dealFrame } from '../utils';
import { createFiltersEnumMap, FiltersEnum } from '../utils/filterUtils';
import KfTradingDataTable from '@kungfu-trader/kungfu-app/src/renderer/components/public/KfTradingDataTable.vue';
import FrameFilters from './FrameFilters.vue';
import { useJournalStore } from '../store/journalStore';
// import { SessionStatusEnum } from '@kungfu-trader/kungfu-js-api/typings/enums';

const props = withDefaults(
  defineProps<{
    currentSession: KungfuApi.SessionResolved | null;
    currentLocation: KungfuApi.KfLocation | null;
    beginTime: bigint;
    endTime: bigint;
    currentTime: bigint;
    isTimeContinue: boolean;
    locationMap: Record<string, string>;
    isExternalUpdate: boolean;
    currentTimeRangeData: {
      range: [bigint, bigint];
      reload: boolean;
    };
  }>(),
  {},
);

const emit = defineEmits<{
  (e: 'changeTimeRange', range: [bigint, bigint]): void;
  (e: 'externalUpdate', value: boolean): void;
  (e: 'updateCurrentTime', value: bigint): void;
}>();

// watchEffect(() => {
//   console.log('locations', props.locationMap);
// });

type dataResolvedType = {
  children?: dataResolvedType;
  key: string;
  title: string;
}[];

const cacheFrameDataList = shallowRef<KungfuApi.FrameResolved[]>([]);

const journalStore = useJournalStore();
const frameColumns = getFrameColumns();
const simpleImage = Empty.PRESENTED_IMAGE_SIMPLE;
const loadingJournal = ref(false);
const dataChangeByCurrentTime = ref(false);
const currentFramesId = ref<number>(-1);
const frameFilter = ref();
let tracerFrame: KungfuApi.Tracer | null = null;
const sliceable = ref(true);
const readEvent = ref(true);
const writeEvent = ref(true);
const frameDataList = shallowRef<KungfuApi.FrameResolved[]>([]);
const frameFiltersReg = ref(createFiltersEnumMap(/.*/));
const msgMap = ref<string[]>([]);

const frameDataListResolved = computed(() => {
  let newRrameList: KungfuApi.FrameResolved[] = [];

  newRrameList = frameDataList.value.filter((item) => {
    // if (msgMap.value.length === 0) {
    //   return true;
    // }
    // return msgMap.value.includes(longfist.msgTypes[item.msgType]);
    return true;
  });

  if (newRrameList.length <= 0) return [];
  console.log('newRrameList', newRrameList, newRrameList.length);

  if (newRrameList.length >= 10000) {
    emit('changeTimeRange', [
      newRrameList[0].genTime,
      newRrameList[newRrameList.length - 1].genTime,
    ]);
  }
  if (
    !props.currentTime ||
    props.currentTime === 0n ||
    props.currentTime > newRrameList[0].genTime
  ) {
    emit('updateCurrentTime', newRrameList[0].genTime);
  }
  return newRrameList;
});

const framesMap = shallowRef<Record<string, KungfuApi.FrameResolved>>({});

const visible = ref(false);
const excludeRowData = [
  // 'stringMsgType',
  // 'sourceName',
  // 'destName',
  'data',
  'sourceToDest',
];

const currentRowDataResolved = computed(() => {
  const currentRowData = framesMap.value[currentFramesId.value];
  console.log(
    'currentRowData',
    currentRowData,
    currentFramesId.value,
    framesMap.value,
  );
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
            ? currentRowData.stringMsgType
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
  async (newSession, oldSession) => {
    console.log('currentSessionLoad', props.currentSession);
    if (newSession && newSession !== oldSession) {
      isLoading = false;
      msgMap.value = [];
      framesMap.value = {};

      loadingJournal.value = true;

      tracerFrame = tracer(
        props.currentSession as KungfuApi.KfLocation,
        readEvent.value,
        writeEvent.value,
        newSession.begin_time,
        newSession.end_time,
      );

      await loadFrameData(
        newSession,
        newSession.begin_time,
        newSession.end_time,
        false,
      );
      dataChangeByCurrentTime.value = true;
    }
  },
);

watch(
  () => props.currentTime,
  async () => {
    if (dataChangeByCurrentTime.value && props.currentSession) {
      console.log(
        'currentTimeLoad',
        props.currentTime,
        tracerFrame?.currentFrame().genTime(),
        tracerFrame?.seekToTime(props.currentTime),
        tracerFrame?.currentFrame().genTime(),
      );
      await loadFrameData(
        props.currentSession as KungfuApi.SessionResolved,
        props.currentSession.begin_time,
        props.currentSession.end_time,
        false,
      );
    }
  },
);
// watchEffect(() => {
//   console.log('变化', props.isExternalUpdate);
// });

// watch(
//   () => props.currentTimeRangeData,
//   (newRangeData) => {
//     if (props.isExternalUpdate && (total >= 9999 || !sliceable.value)) {
//       emit('externalUpdate', false);

//       total = 0;
//       isLoading = false;
//       console.log('退出', {
//         total,
//         isLoading,
//         isExternalUpdate: props.isExternalUpdate,
//       });
//     }
//     console.log('newRangeData', {
//       newRangeData,
//       isLoading: isLoading,
//       isExternalUpdate: props.isExternalUpdate,
//       total: total,
//     });
//     if (props.currentSession) {
//       if (newRangeData.range[0] && newRangeData.range[1]) {
//         if (!props.isExternalUpdate && total >= 10000) {
//           total = 0;
//         }
//         if (newRangeData.reload) {
//           cacheId = 0;
//         }
//         if (!props.isExternalUpdate && total < 9999)
//           loadFrameData(
//             props.currentSession as KungfuApi.SessionResolved,
//             newRangeData.range[0],
//             newRangeData.range[1],
//             !newRangeData.reload,
//           );
//       }
//     }
//   },
//   { deep: true },
// );

// let journalReader: KungfuApi.AssembleReader | null = null;
// let lastReaderArgs = {
//   sessionId: 0,
//   startTime: 0n,
//   endTime: 0n,
// };

// const EVERY_COUNT = 10;
// const LIMIT_COUNT = 1000;

// const checkReaderArgs = (args: {
//   sessionId: number;
//   startTime: bigint;
//   endTime: bigint;
// }) => {
//   if (
//     args.sessionId === lastReaderArgs.sessionId &&
//     args.startTime === lastReaderArgs.startTime &&
//     args.endTime === lastReaderArgs.endTime
//   ) {
//     return false;
//   } else {
//     lastReaderArgs = args;
//     return true;
//   }
// };

const getDataResolved = (data: object): dataResolvedType | null => {
  if (data !== null) {
    const result: dataResolvedType = [];
    Object.keys(data).forEach((item) => {
      const obj: { title: string; key: string } = { title: '', key: '' };
      obj.title = `${item} : ${data[item]}`;
      obj.key = item;
      result.push(obj);
    });
    return result;
  }
  return null;
};

let isLoading = false;
const loadFrameData = (
  session: KungfuApi.SessionResolved,
  startTime: bigint,
  endTime: bigint,
  checking = false,
  oldTracerFrame?: KungfuApi.Tracer | null,
) => {
  if (isLoading && !oldTracerFrame) return;
  isLoading = true;

  if (!readEvent.value && !writeEvent.value) {
    frameDataList.value = [];
    cacheFrameDataList.value = [];
    return;
  }
  // const sessionId = session.index;
  // if (!checkReaderArgs({ sessionId, startTime, endTime }) && !turn) return;

  const curFramesMap = {};
  let count = 0;
  let newTotal = 0;
  const runner = async (): Promise<KungfuApi.FrameResolved[]> => {
    if (!tracerFrame) return Promise.resolve([]);
    let frame: KungfuApi.Frame<'func'> = tracerFrame.currentFrame();

    while (count < 1000 && tracerFrame && tracerFrame.dataAvailable()) {
      frame = tracerFrame.currentFrame();

      if (frame) {
        const dataResolved: dataResolvedType = [
          { children: [], key: 'root-start', title: '{' },
          { key: 'root-end', title: '}' },
        ];
        const data = frame.data();
        const dataChildren = getDataResolved(data);
        if (dataChildren !== undefined && dataChildren !== null) {
          if (!dataResolved[0].children) dataResolved[0].children = [];
          dataResolved[0].children = dataChildren;
        }
        const curFrameData: KungfuApi.Frame = {
          id: newTotal,
          dataLength: frame.dataLength(),
          genTime: frame.genTime(),
          triggerTime: frame.triggerTime(),
          msgType: frame.msgType(),
          stringMsgType: longfist.msgTypes[frame.msgType()],
          source: frame.source(),
          dest: frame.dest(),
          data: data,

          dataResolved: dataResolved,
        };

        curFrameData.destName = props.locationMap[curFrameData.dest];
        curFrameData.sourceName = props.locationMap[curFrameData.source];
        curFrameData.sourceToDest = `${curFrameData.sourceName} -> ${curFrameData.destName}`;
        const curFrameDataResolved = dealFrame(curFrameData);
        curFramesMap[curFrameDataResolved.id] = curFrameDataResolved;
        framesMap.value[curFrameDataResolved.id] = curFrameDataResolved;
        frameFilter.value?.addOption(FiltersEnum.MSG_TYPE, [
          {
            label: curFrameDataResolved.stringMsgType,
            value: curFrameDataResolved.msgType + '',
          },
        ]);

        console.log('traceReader', {
          newTotal,
          curFrameData,
          dataChildren,
          frameFilter: frameFilter.value,
          location: props.currentLocation,
          session: props.currentSession,
          tracerFrame,
          count,
          traceFrame: tracerFrame.dataAvailable(),
          tracerNext: [
            tracerFrame.next(),
            tracerFrame.currentFrame(),
            tracerFrame.dataAvailable(),
          ],
          isLoading,
        });
        // tracerFrame.next();
        ++newTotal;
        ++count;
      }
    }
    if (!tracerFrame.dataAvailable() || newTotal > 9999) {
      console.log('结束', {
        checking,
        count,

        tracerFrame,
        dataAvailable: tracerFrame.dataAvailable(),
        curFramesMap,
        isLoading,
      });
      sliceable.value = tracerFrame.dataAvailable();
      return Object.values(curFramesMap);
    } else {
      count = 0;
      console.log('继续', count, tracerFrame, tracerFrame.dataAvailable());
      return new Promise<KungfuApi.FrameResolved[]>((resolve) => {
        requestAnimationFrame(async () => {
          const res = await runner();
          resolve(res);
        });
      });
    }
  };

  return runner().then((res) => {
    loadingJournal.value = false;
    if (res.length <= 0) return;
    if (checking) {
      frameDataList.value.push(...res);
      currentFramesId.value = frameDataList.value[0]?.id;
      journalStore.setCurrentSessionFrames(res, false);
    } else {
      // isFrameCache.value = false;
      frameDataList.value = res;

      currentFramesId.value = frameDataList.value[0]?.id;
      // isFrameCache.value = false;
      // cacheFrameDataList.value = [];

      journalStore.setCurrentSessionFrames(res, true);
    }
    console.log(
      'journalthen',
      res,
      frameDataListResolved.value,
      msgMap.value,
      checking,
    );

    // if (total < 10000)
    isLoading = false;
    // return sleep(1000).then(() => {
    //   isLoading = false;
    //   if (total >= LIMIT_COUNT) {
    //     loadFrameData(session, startTime, endTime, true);
    //   }
    // });
  });
};

const handleOpenFrameDetail = ({ row }) => {
  currentFramesId.value = row.id;
  visible.value = true;
};

const onFiltersApply = (
  filtersFormState: Record<FiltersEnum, string[]>,
  read: boolean,
  write: boolean,
) => {
  readEvent.value = read;
  writeEvent.value = write;
  isLoading = false;
  msgMap.value = filtersFormState[FiltersEnum.MSG_TYPE];
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
  loadFrameData(
    props.currentSession as KungfuApi.SessionResolved,
    props.currentTimeRangeData.range[0],
    props.currentTimeRangeData.range[1],
    false,
  );

  console.log(
    '过滤onFiltersApply',
    filtersFormState,
    [props.currentTimeRangeData.range[0], props.currentTimeRangeData.range[1]],
    frameDataListResolved.value.length,
    read,
    write,
  );
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
    overflow-x: overlay;
    display: flex;
    align-items: center;
    justify-content: center;
    flex-wrap: nowrap;

    &::-webkit-scrollbar {
      height: 4px;
    }

    .kf-journal-bar-title {
      white-space: nowrap;
      font-size: 14px;
      margin-right: 16px;
    }

    .ant-form-inline {
      flex-wrap: nowrap;
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

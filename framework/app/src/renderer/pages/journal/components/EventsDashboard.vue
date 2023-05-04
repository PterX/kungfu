<template>
  <div class="kf-journal-events__wrap">
    <div class="kf-journal-filters-bar">
      <div class="kf-journal-bar-title">
        {{
          `数据时间范围：${dealKfTime(dataStartTime[0])}-${dealKfTime(
            dataStartTime[1],
          )}`
        }}
      </div>
      <FrameFilters
        ref="frameFilter"
        :location-map="locationMap"
        :current-location="currentLocation"
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
import { ref, computed, watch, shallowRef, nextTick } from 'vue';
import { Empty } from 'ant-design-vue';
import { longfist, tracer } from '@kungfu-trader/kungfu-js-api/kungfu';
import { getFrameColumns } from '../config';
import { dealFrame } from '../utils';
import { createFiltersEnumMap, FiltersEnum } from '../utils/filterUtils';
import KfTradingDataTable from '@kungfu-trader/kungfu-app/src/renderer/components/public/KfTradingDataTable.vue';
import { dealKfTime } from '@kungfu-trader/kungfu-js-api/kungfu';
import FrameFilters from './FrameFilters.vue';
import { useJournalStore } from '../store/journalStore';

// import { debounce } from 'lodash';

const props = withDefaults(
  defineProps<{
    currentSession: KungfuApi.SessionResolved | null;
    currentLocation: KungfuApi.KfLocation | null;
    beginTime: bigint;
    endTime: bigint;
    nowTime: bigint;
    currentTime: bigint;

    locationMap: Record<string, string>;
  }>(),
  {},
);

const emit = defineEmits<{
  (e: 'updateCurrentTime', value: bigint): void;
}>();

type dataResolvedType = {
  children?: dataResolvedType;
  key: string;
  title: string;
}[];

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
const msgMap = new Map<string, boolean>();
const isfilter = ref(false);
const stopLoading = ref(false);
const dataStartTime = ref<[bigint, bigint]>([props.beginTime, props.beginTime]);

const frameDataListResolved = computed(() => {
  if (frameDataList.value.length <= 0) {
    return [];
  }

  if (frameDataList.value.length <= 10000) {
    // stopLoading.value = true;
    // isfilter.value = false;
    // eslint-disable-next-line vue/no-side-effects-in-computed-properties
    dataStartTime.value = [
      props.currentTime,
      frameDataList.value[frameDataList.value.length - 1].genTime,
    ];
    return frameDataList.value;
  } else {
    // clearTimeout(timer);
    // stopLoading.value = false;
    // isfilter.value = false;
    return frameDataList.value.slice(0, 10000);
  }
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
let timer;
// async function loadFrameDataWithAnimationFrame() {
//   await loadFrameData(props.currentSession as KungfuApi.SessionResolved, true);

//   if (stopLoading.value) {
//     timer = setTimeout(async () => {
//       nextTick(() => {});
//     }, 10000);
//   }
// }

watch(
  () => props.currentSession,
  async (newSession, oldSession) => {
    if (newSession && newSession !== oldSession) {
      dataChangeByCurrentTime.value = false;
      msgMap.clear();
      framesMap.value = {};
      // clearTimeout(timer);
      stopLoading.value = false;
      frameDataList.value = [];

      tracerFrame = tracer(
        props.currentSession as KungfuApi.KfLocation,
        readEvent.value,
        writeEvent.value,
        newSession.begin_time,
        newSession.end_time,
      );
      frameFilter.value?.addOption(FiltersEnum.MSG_TYPE, [{}], true);

      await loadFrameData(newSession, false);

      dataChangeByCurrentTime.value = true;

      emit('updateCurrentTime', props.beginTime);
    }
  },
);

let timeGoOn = ref(false);
watch(
  () => props.currentTime,
  async () => {
    if (dataChangeByCurrentTime.value && props.currentSession) {
      clearTimeout(timer);
      stopLoading.value = false;
      frameFilter.value?.addOption(FiltersEnum.MSG_TYPE, [{}], true);
      await doLoad();
      msgMap.clear();
      emit('updateCurrentTime', props.currentTime);
    }
  },
);
// const debouncedCallback = debounce(() => {
//   if (timeGoOn.value) {
//     loadFrameData(props.currentSession as KungfuApi.SessionResolved, true);
//   }
// }, 5000);

// watch(
//   () => props.nowTime,
//   () => {
//     debouncedCallback();
//   },
// );
let timer1: any = null;

const doLoad = () => {
  if (timer1 === null) {
    tracerFrame?.seekToTime(props.currentTime);

    loadFrameData(props.currentSession as KungfuApi.SessionResolved);
    timer1 = setTimeout(() => {
      timer1 = null;
    }, 500);
  }
};

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

const setLoadingJournal = async (value: boolean) => {
  loadingJournal.value = value;
  await nextTick();
};

const loadFrameData = (
  session: KungfuApi.SessionResolved,
  checking = false,
  record = true,
) => {
  if (!readEvent.value && !writeEvent.value) {
    frameDataList.value = [];
    return;
  }
  setLoadingJournal(true);
  const curFramesMap = {};
  let count = 0;
  let newTotal = 0;
  const drain = async (): Promise<KungfuApi.FrameResolved[]> => {
    if (!tracerFrame) return Promise.resolve([]);
    let frame: KungfuApi.Frame<'func'> = tracerFrame.currentFrame();

    while (count < 1000 && tracerFrame && tracerFrame.dataAvailable()) {
      frame = tracerFrame.currentFrame();

      if (frame) {
        const dataResolved: dataResolvedType = [
          { children: [], key: 'root-start', title: '{' },
          { key: 'root-end', title: '}' },
        ];
        const msgType = frame.msgType();
        const stringMsgType = longfist.msgTypes[msgType];
        if (isfilter.value) {
          if (msgMap.size > 0) {
            if (!msgMap.has(stringMsgType)) {
              tracerFrame.next();
              continue;
            }
          }
        } else if (Number(frame.msgType()) >= 10000) {
          tracerFrame.next();
          continue;
        }
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
          msgType: msgType,
          stringMsgType: stringMsgType,
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
        ++newTotal;
        ++count;
        if (record) {
          frameFilter.value?.addOption(FiltersEnum.MSG_TYPE, [
            {
              label: curFrameDataResolved.stringMsgType,
              value: curFrameDataResolved.msgType + '',
            },
          ]);
        }
        tracerFrame.next();
      }
    }
    if (!tracerFrame.dataAvailable() || newTotal > 9999) {
      sliceable.value = tracerFrame.dataAvailable();
      return Object.values(curFramesMap);
    } else {
      count = 0;
      return new Promise<KungfuApi.FrameResolved[]>((resolve) => {
        requestAnimationFrame(async () => {
          const res = await drain();
          resolve(res);
        });
      });
    }
  };

  return drain().then((res) => {
    loadingJournal.value = false;
    if (checking) {
      frameDataList.value.push(...res);
      currentFramesId.value = frameDataList.value[0]?.id;
      journalStore.setCurrentSessionFrames(res, false);
    } else {
      frameDataList.value = res;
      currentFramesId.value = frameDataList.value[0]?.id;
      journalStore.setCurrentSessionFrames(res, true);
    }

    // console.log('then', res, frameDataListResolved.value);

    if (
      !dataChangeByCurrentTime.value &&
      !isfilter.value &&
      frameDataList.value.length < 10000
    ) {
      timeGoOn.value = true;
    } else {
      timeGoOn.value = false;
    }
  });
};

const handleOpenFrameDetail = ({ row }) => {
  currentFramesId.value = row.id;
  visible.value = true;
};

const onFiltersApply = async (
  filtersFormState: Record<FiltersEnum, string[]>,
  read: boolean,
  write: boolean,
) => {
  readEvent.value = read;
  writeEvent.value = write;
  msgMap.clear();
  filtersFormState[FiltersEnum.MSG_TYPE].forEach((item) => {
    if (Number.isNaN(Number(item))) {
      msgMap.set(item, true);
    } else {
      msgMap.set(longfist.msgTypes[Number(item)], true);
    }
  });
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
  isfilter.value = true;
  tracerFrame?.seekToTime(props.currentTime);
  frameDataList.value = [];
  await loadFrameData(
    props.currentSession as KungfuApi.SessionResolved,
    false,
    false,
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

<template>
  <div class="kf-journal-events__wrap">
    <div class="kf-journal-filters-bar">
      <div class="kf-journal-bar-title">
        <span>{{ `${$t('journalConfig.time_range')}: ` }}</span>
        <a-button
          class="kf-time-btn__decrease"
          type="normal"
          @mousedown.prevent
          @click="decreaseTimestamp"
        >
          <template #icon>
            <minus-outlined />
          </template>
        </a-button>
        <a-input
          ref="inputRef"
          type="text"
          size="large"
          v-model:value="inputStartTime"
          @blur="handleStartTimeBlur"
          @keyup.enter="handleStartTimeEnter"
          autofocus
          :placeholder="$t('journalConfig.please_input_time')"
          style="width: 128px"
        />
        <a-button
          class="kf-time-btn__increase"
          type="normal"
          @mousedown.prevent
          @click="increaseTimestamp"
        >
          <template #icon>
            <plus-outlined />
          </template>
        </a-button>
        <span>{{ ` - ${dealKfTime(dataStartTime[1])}` }}</span>
      </div>

      <FrameFilters
        ref="frameFilter"
        @apply-filters="onFiltersApply"
      ></FrameFilters>
    </div>
    <div class="kf-journal-frame__wrap">
      <KfTradingDataTable
        :data-source="frameDataList"
        :columns="frameColumns"
        key-field="id"
        :resizable="false"
        :custom-row-class="dealRowClassName"
        @resetScrollTop="resetToTop = $event"
        @click-cell="handleOpenFrameDetail"
        @click-row="handleOpenFrameDetail"
        @onScrollToTop="handleScrollToTop"
        @onScrollToBottom="handleScrollToBottom"
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
                color: '#ffffffd9',
                backgroundColor: dealTagBackgroudColor(
                  item.msgTypeResolved.color || 'rgb(158, 158, 158)',
                ),
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
            :selectable="true"
            default-expand-all
          >
            <template #title="{ title }">
              <span class="tree-node-title">{{ title }}</span>
            </template>
          </a-tree>
        </a-card>
      </template>
      <template v-else>
        <a-empty :image="simpleImage"></a-empty>
      </template>
    </a-drawer>
  </div>
</template>

<script lang="ts" setup>
import { ref, computed, watch, shallowRef, onMounted } from 'vue';
import { Empty } from 'ant-design-vue';
import { PlusOutlined, MinusOutlined } from '@ant-design/icons-vue';
import { longfist, tracer } from '@kungfu-trader/kungfu-js-api/kungfu';
import { getFrameColumns } from '../config';
import { dealFrame } from '../utils';
import { MsgType } from '@kungfu-trader/kungfu-app/src/typings/enums';
import { FiltersEnum, useMsgTypesMap } from '../utils/filterUtils';
import KfTradingDataTable from '@kungfu-trader/kungfu-app/src/renderer/components/public/KfTradingDataTable.vue';
import { dealKfTime } from '@kungfu-trader/kungfu-js-api/kungfu';
import { delayMilliSeconds } from '@kungfu-trader/kungfu-js-api/utils/busiUtils';
import { SessionStatusEnum } from '@kungfu-trader/kungfu-js-api/typings/enums';
import FrameFilters from './FrameFilters.vue';
import { debounce } from 'lodash';

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

const FRAME_LIST_SPLIT = 100;
const SCALE = 1000000;
const HUNDRED_MILLISECONDS = 100000000;
const DEFAULT_LIST_SIZE = 10000;
const msgDetailsArray = [
  MsgType.Order,
  MsgType.OrderInput,
  MsgType.Position,
  MsgType.Quote,
  MsgType.Trade,
];

const inputRef = ref<HTMLInputElement>({} as HTMLInputElement);
const frameColumns = getFrameColumns();
const simpleImage = Empty.PRESENTED_IMAGE_SIMPLE;
const loadingJournal = ref(false);
const dataChangeByCurrentTime = ref(false);
const currentFramesId = ref<number>(-1);
const frameFilter = ref();
let currentTracer: KungfuApi.Tracer | null = null;
const readEvent = ref(true);
const writeEvent = ref(true);
const frameDataList = shallowRef<KungfuApi.FrameResolved[]>([]);
const msgTypesMapManager = useMsgTypesMap();
let requestBreakLoadingDataWhile = false;
let isLoadingFrames = false;

const dataStartTime = ref<[bigint, bigint]>([
  props.currentTime,
  props.beginTime,
]);
const inputStartTime = ref<string>(dealKfTime(props.currentTime));
const colorMap = {
  blue: 'rgb(24, 144, 255)',
  green: 'rgb(82, 196, 26)',
  '#FAAD14': 'rgb(250, 173, 20)',
  purple: 'rgb(83, 29, 171)',
};

watch(
  () => frameDataList.value.length,
  (len) => {
    if (len === 0) {
      if (props.currentSession?.status === SessionStatusEnum.Running) {
        dataStartTime.value = [props.currentTime, props.nowTime];
      } else {
        dataStartTime.value = [props.currentTime, props.endTime];
      }
      return;
    }

    dataStartTime.value = [
      props.currentTime,
      frameDataList.value[frameDataList.value.length - 1].genTime,
    ];
  },
);

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

const resetToTop = ref<(() => void) | null>(null);

onMounted(() => {
  msgTypesMapManager.init();
});

const handleScrollToTop = () => {
  //TODO on scroll to top event;
};

const handleScrollToBottom = async () => {
  await delayMilliSeconds(0);
  await loadFrameData(true);
};

const handleStartTimeBlur = () => {
  validateAndUpdateStartTime();
};
const handleStartTimeEnter = () => {
  validateAndUpdateStartTime();
};

const convertToTimestamp = (timeStr) => {
  if (/^\d{10,19}$/.test(timeStr)) {
    const lengthDifference = 19 - timeStr.length;
    const scaleFactor = BigInt(Math.pow(10, lengthDifference));

    return BigInt(timeStr) * scaleFactor;
  } else {
    const [hours, minutes, fullSeconds] = timeStr.split(':');
    const [seconds, milliseconds] = fullSeconds.includes('.')
      ? fullSeconds.split('.')
      : [fullSeconds, '000'];
    const currentTime = new Date();
    currentTime.setHours(+hours, +minutes, +seconds, +milliseconds);
    return BigInt(currentTime.getTime()) * BigInt(SCALE);
  }
};
const validateAndUpdateStartTime = async () => {
  const timeRegex = /^(\d{10,19}|(\d{2}:\d{2}:\d{2}(\.\d{3})?))$/;
  if (timeRegex.test(inputStartTime.value)) {
    const newStartTime = convertToTimestamp(inputStartTime.value);
    if (newStartTime >= props.nowTime) {
      dataStartTime.value[0] = props.nowTime;
    } else if (newStartTime <= props.beginTime) {
      dataStartTime.value[0] = props.beginTime;
    } else {
      dataStartTime.value[0] = newStartTime;
    }
    if (dataStartTime.value[0] !== props.currentTime) {
      await emit('updateCurrentTime', dataStartTime.value[0]);
    }
  }

  inputStartTime.value = dealKfTime(dataStartTime.value[0]);
};

const modifyTimestamp = (isIncrease) => {
  const timeRegex = /^(\d{10,19}|(\d{2}:\d{2}:\d{2}(\.\d{3})?))$/;
  if (timeRegex.test(inputStartTime.value)) {
    if (/^\d{10,19}$/.test(inputStartTime.value)) {
      const lengthDifference = 19 - inputStartTime.value.length;
      const scaleFactor = BigInt(Math.pow(10, lengthDifference));
      const currentTimestamp = BigInt(inputStartTime.value) * scaleFactor;
      const adjustment =
        BigInt(HUNDRED_MILLISECONDS) * BigInt(isIncrease ? 1 : -1);
      const newTimestamp = currentTimestamp + adjustment;
      inputStartTime.value = newTimestamp.toString();
    } else {
      const [hours, minutes, fullSeconds] = inputStartTime.value.split(':');
      const [seconds, milliseconds] = fullSeconds.includes('.')
        ? fullSeconds.split('.')
        : [fullSeconds, '000'];
      const currentTime = new Date();
      currentTime.setHours(+hours, +minutes, +seconds, +milliseconds);
      const adjustment =
        BigInt(HUNDRED_MILLISECONDS) * BigInt(isIncrease ? 1 : -1);
      inputStartTime.value = dealKfTime(
        BigInt(currentTime.getTime()) * BigInt(SCALE) + adjustment,
      );
    }
  } else {
    inputStartTime.value = '';
  }
  inputRef.value.focus();
  validateAndUpdateStartTime();
};

const increaseTimestamp = () => modifyTimestamp(true);
const decreaseTimestamp = () => modifyTimestamp(false);

watch(
  () => props.currentSession,
  (newSession) => {
    if (newSession) {
      dataChangeByCurrentTime.value = false;
      framesMap.value = {};
      currentTracer = tracer(
        props.currentSession as KungfuApi.KfLocation,
        readEvent.value,
        writeEvent.value,
        newSession.begin_time,
        newSession.end_time,
      );
      dataChangeByCurrentTime.value = true;
      emit('updateCurrentTime', props.beginTime);
    }
  },
  {
    deep: true,
  },
);

watch(
  () => props.currentTime,
  () => {
    inputStartTime.value = dealKfTime(props.currentTime);
    if (dataChangeByCurrentTime.value && props.currentSession) {
      initLoad();
      emit('updateCurrentTime', props.currentTime);
    }
  },
);

const initLoad = debounce(async () => {
  isLoadingFrames && (requestBreakLoadingDataWhile = true);
  await delayMilliSeconds(0);
  frameDataList.value = [];
  loadingJournal.value = true;
  await delayMilliSeconds(0);
  currentTracer?.seekToTime(props.currentTime);
  await loadFrameData();
  resetToTop.value?.();
  requestBreakLoadingDataWhile = false;
}, 500);

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

const loadFrameData = async (loadmore = false) => {
  let newTotal = frameDataList.value.length;
  const drain = async (): Promise<KungfuApi.FrameResolved[]> => {
    if (!currentTracer) return Promise.resolve([]);
    let frame: KungfuApi.Frame<'func'> = currentTracer.currentFrame();
    let tempFrames: KungfuApi.FrameResolved[] = [];

    while (
      tempFrames.length < FRAME_LIST_SPLIT &&
      currentTracer &&
      currentTracer.dataAvailable() &&
      !requestBreakLoadingDataWhile
    ) {
      frame = currentTracer.currentFrame();
      if (!frame) {
        break;
      }

      const dataResolved: dataResolvedType = [
        { children: [], key: 'root-start', title: '{' },
        { key: 'root-end', title: '}' },
      ];
      const msgType = frame.msgType();
      const stringMsgType = longfist.msgTypes[msgType];

      if (
        msgTypesMapManager.msgTypesMap.size > 0 &&
        !msgTypesMapManager.msgTypesMap.has(stringMsgType)
      ) {
        currentTracer.next();
        continue;
      }

      const data = frame.data();
      const dataChildren = getDataResolved(data as object);
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
        currentFrameId: currentTracer.currentFrameId(),
        currentPageId: currentTracer.currentPageId(),
        stringMsgType: stringMsgType,
        msgDetails: msgDetailsArray.includes(Number(msgType))
          ? frame.dataAsString().slice(1, -1)
          : '',
        source: frame.source(),
        dest: frame.dest(),
        data: frame.dataAsString() as string,
        dataResolved: dataResolved,
      };

      curFrameData.destName =
        curFrameData.dest === props.currentSession?.location_uid
          ? 'self'
          : props.locationMap[curFrameData.dest];
      curFrameData.sourceName =
        curFrameData.source === props.currentSession?.location_uid
          ? 'self'
          : props.locationMap[curFrameData.source];
      curFrameData.sourceToDest = `${curFrameData.sourceName} -> ${curFrameData.destName}`;

      const curFrameDataResolved = dealFrame(curFrameData);
      const { msgDetails, ...restFrameData } = curFrameDataResolved;
      framesMap.value[curFrameDataResolved.id] = restFrameData;
      tempFrames.push(curFrameDataResolved);
      currentTracer.next();
      ++newTotal;
    }

    loadingJournal.value && (loadingJournal.value = false);
    if (requestBreakLoadingDataWhile) {
      frameDataList.value = [];
      return [];
    } else {
      frameDataList.value = [...frameDataList.value, ...tempFrames];
      tempFrames = [];
    }

    if (
      !currentTracer.dataAvailable() ||
      frameDataList.value.length >= DEFAULT_LIST_SIZE ||
      newTotal >= DEFAULT_LIST_SIZE ||
      loadmore
    ) {
      return frameDataList.value;
    } else {
      return new Promise<KungfuApi.FrameResolved[]>((resolve) => {
        requestAnimationFrame(async () => {
          const frames = await drain();
          resolve(frames);
        });
      });
    }
  };

  isLoadingFrames = true;
  return drain().then((_: KungfuApi.FrameResolved[]) => {
    isLoadingFrames = false;
    currentFramesId.value = frameDataList.value[0]?.id;
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
  msgTypesMapManager.reset(filtersFormState[FiltersEnum.MSG_TYPE]);
  currentTracer = tracer(
    props.currentSession as KungfuApi.KfLocation,
    readEvent.value,
    writeEvent.value,
    props.currentSession?.begin_time as bigint,
    props.currentSession?.end_time as bigint,
  );
  initLoad();
};

const dealTagBackgroudColor = (colorStr: string) => {
  if (!colorStr || colorStr === 'default') return '';
  let color = colorMap[colorStr];
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
  height: 100%;
  width: 100%;
  display: flex;
  flex-direction: column;
  padding: 4px 8px;
  box-sizing: border-box;
  overflow: hidden;

  .kf-journal-spin {
    .ant-spin-text {
      margin-left: 8px;
    }
  }

  .kf-journal-filters-bar {
    background-color: #1d1d1d;
    padding: 4px 16px;
    margin-bottom: 4px;
    overflow-x: overlay;
    display: flex;
    align-items: center;
    justify-content: space-between;
    flex-wrap: nowrap;

    &::-webkit-scrollbar {
      height: 4px;
    }

    .kf-journal-bar-title {
      height: 32px;
      white-space: nowrap;
      font-size: 14px;
      margin-right: 16px;
      display: flex;
      align-items: center;

      button {
        height: 100%;

        &.kf-time-btn__increase {
          margin-right: 8px;
        }

        &.kf-time-btn__decrease {
          margin-left: 8px;
        }
      }

      input {
        height: 100%;
      }
    }

    .ant-form-inline {
      flex-wrap: nowrap;
      justify-content: space-between;
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

.tree-node-title {
  display: inline-block;
  white-space: normal;
  word-break: break-word;
  max-width: 300px;
  user-select: text;
}
</style>

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
        :channels="channels"
        :read="readEvent"
        :write="writeEvent"
        :selected-msg-types="selectedMsgTypes"
        :selected-channels="selectedChannels"
      ></FrameFilters>
    </div>
    <div class="kf-journal-frame__wrap">
      <KfTradingDataTable
        :data-source="frameDataList"
        :columns="frameColumns"
        key-field="id"
        :resizable="false"
        :custom-row-class="dealRowClassName"
        @resetScrollTop="setResetToTopObject($event)"
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
          <template v-if="column.dataIndex === 'msgTypeName'">
            <a-tag
              :style="{
                color: '#ffffffd9',
                backgroundColor: dealTagBackgroudColor(
                  item.msgTypeResolved.color || 'rgb(158, 158, 158)',
                ),
              }"
            >
              {{ item.msgTypeName }}
            </a-tag>
          </template>
          <template v-else-if="column.dataIndex === 'data'">
            <span v-if="SHOW_DETAIL_MSG_TYPES[+item.msgType]">
              {{ item.data }}
            </span>
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
      :spinning="firstSplitFramesLoading"
      :tip="$t('journalConfig.loading_journal')"
    />
    <a-drawer
      v-model:visible="visible"
      title="Event Frame"
      placement="right"
      :force-render="true"
    >
      <template v-if="frameHeaderForShow">
        <a-card title="Frame Header" style="margin: 35px 0">
          <a-list size="normal" bordered :data-source="frameHeaderForShow">
            <template #renderItem="{ item }">
              <a-list-item>
                <span class="frame-detail-datalist-key">{{ item.key }}</span>
                <span class="frame-detail-datalist-value">
                  {{ item.value }}
                </span>
              </a-list-item>
            </template>
          </a-list>
        </a-card>

        <a-card title="Frame Data" style="margin: 35px 0">
          <a-list size="normal" bordered :data-source="frameDataForShow">
            <template #renderItem="{ item }">
              <a-list-item>
                <span class="frame-detail-datalist-key">{{ item.key }}</span>
                <span class="frame-detail-datalist-value">
                  {{ item.value }}
                </span>
              </a-list-item>
            </template>
          </a-list>
        </a-card>
      </template>
      <template v-else>
        <a-empty :image="simpleImage"></a-empty>
      </template>
    </a-drawer>
  </div>
</template>

<script lang="ts" setup>
import { ref, computed, watch, shallowRef, nextTick, onMounted, getCurrentInstance, onBeforeUnmount } from 'vue';
import { Empty } from 'ant-design-vue';
import { PlusOutlined, MinusOutlined } from '@ant-design/icons-vue';
import { tracer } from '@kungfu-trader/kungfu-js-api/kungfu';
import { getFrameColumns } from '../config';
import { dealFrame, buildFrameHeaderForShow } from '../utils';
import { MsgType } from '@kungfu-trader/kungfu-app/src/typings/enums';
import { useMsgTypesMap, ChannelRecords } from '../utils/filterUtils';
import KfTradingDataTable from '@kungfu-trader/kungfu-app/src/renderer/components/public/KfTradingDataTable.vue';
import { dealKfTime } from '@kungfu-trader/kungfu-js-api/kungfu';
import { SessionStatusEnum } from '@kungfu-trader/kungfu-js-api/typings/enums';
import FrameFilters from './FrameFilters.vue';
import { debounce } from 'lodash';
import { delayMilliSeconds } from '@kungfu-trader/kungfu-js-api/utils/busiUtils';
import { filter } from 'rxjs';

const props = withDefaults(
  defineProps<{
    currentSession: KungfuApi.SessionResolved | null;
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

const app = getCurrentInstance();
const dashboardVisible = ref<boolean>(true);
onMounted(() => {
  if (app?.proxy) {
      const subscription = app?.proxy.$globalBus
        .pipe(filter((e: KfEvent.KfBusEvent) => e.tag === 'resize'))
        .subscribe(async() => {
          dashboardVisible.value = false;
          await nextTick();
          dashboardVisible.value = true;
        });

      onBeforeUnmount(() => {
        subscription.unsubscribe();
      });
    }
});

const FRAME_LIST_SPLIT = 200;
const SCALE = 1000000;
const HUNDRED_MILLISECONDS = 100000000;
const DEFAULT_LIST_SIZE = 2000;
const SHOW_DETAIL_MSG_TYPES = {
  [MsgType.Asset]: true,
  [MsgType.AssetMargin]: true,
  [MsgType.Position]: true,
  [MsgType.Order]: true,
  [MsgType.OrderInput]: true,
  [MsgType.Trade]: true,
  [MsgType.OrderAction]: true,
  [MsgType.OrderActionError]: true,
  [MsgType.BlockMessage]: true,
  [MsgType.Quote]: true,
};

const inputRef = ref<HTMLInputElement>({} as HTMLInputElement);
const frameColumns = getFrameColumns();
const simpleImage = Empty.PRESENTED_IMAGE_SIMPLE;
const firstSplitFramesLoading = ref(false);
const dataChangeByCurrentTime = ref(false);
const currentFramesId = ref<string>('');
const frameFilter = ref();
let currentTracer: KungfuApi.Tracer | null = null;

const frameDataList = shallowRef<KungfuApi.FrameResolved[]>([]);
let requestBreakLoadingDataWhile = false;
let isLoadingFrames = false;

const channels = ref<ChannelRecords>({} as ChannelRecords);
const selectedChannels = ref<string[]>([]);
const { selectedMsgTypes, selectedMsgTypesMap } = useMsgTypesMap();

const readEvent = ref(true);
const writeEvent = ref(true);

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

const visible = ref(false);
const currentRowData = ref<KungfuApi.FrameResolved | null>(null);
const frameHeaderForShow = computed(() => {
  if (!currentRowData.value) return null;
  const frameHeader = buildFrameHeaderForShow(currentRowData.value);
  return Object.entries(frameHeader).map(([key, value]) => {
    return {
      key,
      value,
    };
  });
});
const frameDataForShow = computed(() => {
  if (!currentRowData.value) return [];

  const dataAsString = currentRowData.value.dataAsString.slice(2, -1);
  return dataAsString.split(',"').map((item) => {
    item = '"' + item;
    const pair = item.split(':');
    return { key: pair[0], value: pair[1] };
  });
});

const resetToTop = ref<(() => void) | null>(null);

const setResetToTopObject = (e: (() => void) | undefined) => {
  e && (resetToTop.value = e);
};

const handleScrollToTop = () => {
  //TODO on scroll to top event;
};

const handleScrollToBottom = debounce(async () => {
  console.log('scrolling to bottom');
  if (!props.currentSession) return;
  if (isLoadingFrames) return;
  await delayMilliSeconds(0);
  await loadFrameData(props.currentSession.session_id_origin, true);
}, 25);

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
      channels.value = {};
      frameFilter.value?.resetFilters();
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

watch(
  () => firstSplitFramesLoading.value,
  (newIsLoading, oldIsLoading) => {
    if (!newIsLoading && oldIsLoading) {
      resetToTop.value?.();
    }
  },
);

const initLoad = debounce(async () => {
  console.warn('initLoad');
  if (!props.currentSession) return;
  const sessionIdOrigin = props.currentSession.session_id_origin;
  isLoadingFrames && (requestBreakLoadingDataWhile = true);
  firstSplitFramesLoading.value = true;
  // wait for while looping and break while working
  await delayMilliSeconds(0);
  frameDataList.value = [];
  await nextTick();
  currentTracer?.seekToTime(props.currentTime);
  await loadFrameData(sessionIdOrigin);
  requestBreakLoadingDataWhile = false;
  firstSplitFramesLoading.value = false;
}, 100);

const loadFrameData = async (currentSessionId: string, loadmore = false) => {
  console.warn('loadFrameData, loadmore', loadmore);
  const drain = async (
    sessionId: string,
  ): Promise<KungfuApi.FrameResolved[]> => {
    if (!currentTracer) return Promise.resolve([]);
    let tempFrames: KungfuApi.FrameResolved[] = [];
    let tempCount = 0; // sometimes not enough data in journal, so use this count forbidden long time while
    let totalCount = 0;

    while (
      tempFrames.length < FRAME_LIST_SPLIT &&
      tempCount < FRAME_LIST_SPLIT &&
      currentTracer &&
      currentTracer.dataAvailable() &&
      !requestBreakLoadingDataWhile &&
      props.currentSession &&
      sessionId === props.currentSession.session_id_origin
    ) {
      tempCount++;
      totalCount++;
      const frame = currentTracer.currentFrame();
      if (!frame) {
        break;
      }

      const msgType = frame.msgType();
      if (selectedMsgTypes.value.length > 0 && !selectedMsgTypesMap.value[msgType]) {
        currentTracer.next();
        continue;
      }

      const source = frame.source();
      const dest = frame.dest();
      const pageId = currentTracer.currentPageId();
      const frameId = currentTracer.currentFrameId();
      const curFrameData: KungfuApi.Frame = {
        dataLength: frame.dataLength(),
        genTime: frame.genTime(),
        triggerTime: frame.triggerTime(),
        msgType,
        frameId,
        pageId,
        source,
        dest,
        dataAsString: frame.dataAsString(),
        data: frame.data(),
      };
      const curFrameDataResolved = dealFrame(
        curFrameData,
        props.currentSession,
        props.locationMap,
      );

      currentTracer.next();

      if (!channels.value[curFrameDataResolved.sourceToDest]) {
        channels.value = {
          ...channels.value,
          ...{
            [curFrameDataResolved.sourceToDest]: [
              curFrameDataResolved.source,
              curFrameDataResolved.dest,
            ],
          },
        };
      }

      if (
        selectedChannels.value.length === 0 ||
        selectedChannels.value.includes(curFrameDataResolved.sourceToDest)
      ) {
        tempFrames.push(curFrameDataResolved);
      } else {
        continue; // for future code
      }
    }

    if (
      props.currentSession &&
      sessionId === props.currentSession.session_id_origin &&
      !requestBreakLoadingDataWhile &&
      firstSplitFramesLoading.value
    ) {
      firstSplitFramesLoading.value = false;
    }

    if (
      requestBreakLoadingDataWhile ||
      !(
        props.currentSession &&
        sessionId === props.currentSession.session_id_origin
      )
    ) {
      frameDataList.value = [];
      return [];
    } else {
      frameDataList.value = [...frameDataList.value, ...tempFrames];
    }

    if (
      !currentTracer.dataAvailable() ||
      frameDataList.value.length >= DEFAULT_LIST_SIZE ||
      totalCount >= DEFAULT_LIST_SIZE ||
      loadmore
    ) {
      return frameDataList.value;
    } else {
      return new Promise<KungfuApi.FrameResolved[]>((resolve) => {
        requestAnimationFrame(async () => {
          tempFrames = [];
          tempCount = 0;
          const frames = await drain(sessionId);
          resolve(frames);
        });
      });
    }
  };

  isLoadingFrames = true;
  return drain(currentSessionId).then((_: KungfuApi.FrameResolved[]) => {
    isLoadingFrames = false;
    firstSplitFramesLoading.value = false;
    requestBreakLoadingDataWhile = false;
    currentFramesId.value = frameDataList.value[0]?.id;
  });
};

const handleOpenFrameDetail = async ({ row }) => {
  currentFramesId.value = row.id;
  currentRowData.value = row as KungfuApi.FrameResolved;
  await nextTick();
  visible.value = true;
};

const onFiltersApply = async (
  read: boolean,
  write: boolean,
  afterFilterChannels: string[],
  afterFilterMsgTypes: number[],
) => {
  readEvent.value = read;
  writeEvent.value = write;
  selectedChannels.value = afterFilterChannels;
  selectedMsgTypes.value = afterFilterMsgTypes;
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
    width: 100%;
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
      width: 450px;
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

  .ant-list-item {
    flex-wrap: wrap;
    .frame-detail-datalist-key {
      text-align: left;
      user-select: all;
    }

    .frame-detail-datalist-value {
      flex-wrap: wrap;
      word-break: break-word;
      user-select: all;
    }
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

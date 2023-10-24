<template>
  <div v-if="contentVisible" class="kf-journal-events__wrap">
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
          v-model:value="currentStartTimeInput"
          type="text"
          size="large"
          :placeholder="$t('journalConfig.please_input_time')"
          style="width: 128px"
          @blur="handleStartTimeBlur"
          @keyup.enter="handleStartTimeEnter"
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
        <span>{{ ` - ${dealKfTime(loadedLastFrameTime)}` }}</span>
      </div>

      <FrameFilters
        ref="frameFilter"
        :channels="channels"
        :read="readEvent"
        :write="writeEvent"
        :selected-msg-types="selectedMsgTypes"
        :selected-channels="selectedChannels"
        @apply-filters="onFiltersApply"
      ></FrameFilters>
    </div>
    <div class="kf-journal-frame__wrap">
      <Transition name="fade">
        <div v-show="searchInUsing" class="kf-search-in-table__warp">
          <div class="kf-search-in-table__content">
            <a-input-search
              ref="inputSearchRef"
              v-model:value="searchKeyword"
              class="kf-search-in-table__item"
              :placeholder="$t('keyword_input')"
            />
            <div class="kf-search-in-table__item">
              {{ currentResultIndex }} /
              {{ totalResultCount }}
            </div>
            <div class="kf-search-in-table__item kf-actions__warp">
              <up-outlined
                style="font-size: 14px; margin-left: 0px"
                @click="handleToUpSearchResult"
              />
              <down-outlined
                style="font-size: 14px; margin-left: 8px"
                @click="handleToDownSearchResult"
              />
            </div>
            <a-button @click="searchInUsing = false">
              {{ $t('cancel') }}
            </a-button>
          </div>
        </div>
      </Transition>
      <KfTradingDataTable
        ref="scrollerTableRef"
        :data-source="currentFrameList"
        :columns="frameColumns"
        key-field="id"
        :dynamic="searchInUsing"
        :will-switch-dynamic="true"
        :size-dependencies-fields="['dataAsString']"
        :resizable="false"
        :custom-row-class="dealRowClassName"
        @click-cell="handleClickRow"
        @click-row="handleClickRow"
        @right-click-row="handleOpenFrameDetail"
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
                backgroundColor: dealTagBackgroundColor(
                  item.msgTypeResolved.color || 'rgb(158, 158, 158)',
                ),
              }"
            >
              <span v-html="getItemHtmlResult(item, 'msgTypeName')"></span>
            </a-tag>
          </template>
          <template v-else-if="column.dataIndex === 'data'">
            <span v-if="SHOW_DETAIL_MSG_TYPES[+item.msgType]">
              {{ item.data }}
            </span>
          </template>
          <template v-else-if="column.dataIndex === 'dataAsString'">
            <span v-html="getItemHtmlResult(item, 'dataAsString')"></span>
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
        <a-empty :image="simpleImage" :description="t('empty_text')"></a-empty>
      </template>
    </a-drawer>
  </div>
</template>

<script lang="ts" setup>
import { ref, computed, watch, nextTick, onMounted, onBeforeMount } from 'vue';
import { storeToRefs } from 'pinia';
import { Empty } from 'ant-design-vue';
import {
  PlusOutlined,
  MinusOutlined,
  UpOutlined,
  DownOutlined,
} from '@ant-design/icons-vue';
import { tracer } from '@kungfu-trader/kungfu-js-api/kungfu';
import { getFrameColumns } from '../config';
import {
  dealFrame,
  buildFrameHeaderForShow,
  useResizeFlag,
  getSourceDestMap,
  useNow,
} from '../utils';
import { MsgType } from '@kungfu-trader/kungfu-app/src/typings/enums';
import { useMsgTypesMap, ChannelRecords } from '../utils/filterUtils';
import KfTradingDataTable from '@kungfu-trader/kungfu-app/src/renderer/components/public/KfTradingDataTable.vue';
import { dealKfTime } from '@kungfu-trader/kungfu-js-api/kungfu';
import { SessionStatusEnum } from '@kungfu-trader/kungfu-js-api/typings/enums';
import FrameFilters from './FrameFilters.vue';
import { useJournalStore } from '../store/journalStore';
import {
  delayMilliSeconds,
  debounce,
} from '@kungfu-trader/kungfu-js-api/utils/commonUtils';
import { useScrollerTableSearch } from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/uiUtils';
import VueI18n from '@kungfu-trader/kungfu-js-api/language';

const { t } = VueI18n.global;

const { contentVisible } = useResizeFlag();
const {
  currentSession,
  currentTime,
  currentSessionBeginTime,
  currentSessionEndTime,
  currentFrameList,
  isLoadingFrames,
  selectedChartItem,
  currentFrameId,
} = storeToRefs(useJournalStore());
const {
  setCurrentFrameList,
  setCurrentTime,
  setCurrentLastFrameTime,
  setCurrentFrame,
  setCurrentFrameId,
} = useJournalStore();
const sourceDestMap = getSourceDestMap();
const { now } = useNow();
const scrollerTableRef = ref();
const {
  searchInUsing,
  inputSearchRef,
  searchKeyword,
  currentResultIndex,
  totalResultCount,
  clearSearchState,
  handleToDownSearchResult,
  handleToUpSearchResult,
  getItemHtmlResult,
} = useScrollerTableSearch(
  currentFrameList,
  'id',
  ['msgTypeName', 'dataAsString'],
  scrollerTableRef,
);

const FRAME_LIST_SPLIT = 200;
const SCALE = 1000000;
const HUNDRED_MILLISECONDS = 100000000;
const DEFAULT_LIST_SIZE = 10000;
const SHOW_DETAIL_MSG_TYPES = {
  [MsgType.Asset]: true,
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
const frameColumns = computed(() => getFrameColumns(searchInUsing.value));
const simpleImage = Empty.PRESENTED_IMAGE_SIMPLE;
const firstSplitFramesLoading = ref(false);
const frameFilter = ref();
let currentTracer: KungfuApi.Tracer | null = null;

let requestBreakLoadingDataWhile = false;

const channels = ref<ChannelRecords>({} as ChannelRecords);
const selectedChannels = ref<string[]>([]);
const { selectedMsgTypes, selectedMsgTypesMap } = useMsgTypesMap();

const readEvent = ref(true);
const writeEvent = ref(true);

const currentStartTimeInput = ref<string>(dealKfTime(currentTime.value));
const colorMap = {
  blue: 'rgb(24, 144, 255)',
  green: 'rgb(82, 196, 26)',
  '#FAAD14': 'rgb(250, 173, 20)',
  purple: 'rgb(83, 29, 171)',
};

watch(searchInUsing, (n, o) => {
  if (n !== o) {
    if (scrollerTableRef.value) {
      const [startIndex] = scrollerTableRef.value.getVisibleIndexRange();

      if (startIndex !== undefined && startIndex > -1) {
        nextTick(() => {
          scrollerTableRef.value.scrollToItem(startIndex);
        });
      }
    }
  }
});

const loadedLastFrameTime = computed(() => {
  if (currentFrameList.value.length) {
    return currentFrameList.value[currentFrameList.value.length - 1].genTime;
  } else if (currentSession.value?.status === SessionStatusEnum.Running) {
    return now.value;
  } else {
    return currentSessionEndTime.value;
  }
});

watch(
  () => loadedLastFrameTime.value,
  (nano) => {
    setCurrentLastFrameTime(nano);
  },
);

watch(
  () => selectedChartItem.value,
  (newScrollToItem) => {
    if (scrollerTableRef.value) {
      scrollerTableRef.value.scrollToItem(newScrollToItem);
    }
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

const handleScrollToTop = () => {
  //TODO on scroll to top event;
  if (scrollerTableRef.value) {
    scrollerTableRef.value.scrollToItem(0);
  }
};

const handleScrollToBottom = debounce(async () => {
  console.warn('scrolling to bottom');
  if (!currentSession.value) return;
  if (isLoadingFrames.value) return;
  // wait for while looping and break while working
  await delayMilliSeconds(0);
  await loadFrameData(currentSession.value.index, true);
}, 50);

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
  if (timeRegex.test(currentStartTimeInput.value)) {
    const newStartTime = convertToTimestamp(currentStartTimeInput.value);
    if (newStartTime >= now.value) {
      currentStartTimeInput.value = dealKfTime(now.value);
    } else if (newStartTime <= currentSessionBeginTime.value) {
      currentStartTimeInput.value = dealKfTime(currentSessionBeginTime.value);
    }

    setCurrentTime(newStartTime);
  }
};

const modifyTimestamp = (isIncrease) => {
  const timeRegex = /^(\d{10,19}|(\d{2}:\d{2}:\d{2}(\.\d{3})?))$/;
  if (timeRegex.test(currentStartTimeInput.value)) {
    if (/^\d{10,19}$/.test(currentStartTimeInput.value)) {
      const lengthDifference = 19 - currentStartTimeInput.value.length;
      const scaleFactor = BigInt(Math.pow(10, lengthDifference));
      const currentTimestamp =
        BigInt(currentStartTimeInput.value) * scaleFactor;
      const adjustment =
        BigInt(HUNDRED_MILLISECONDS) * BigInt(isIncrease ? 1 : -1);
      const newTimestamp = currentTimestamp + adjustment;
      currentStartTimeInput.value = newTimestamp.toString();
    } else {
      const [hours, minutes, fullSeconds] =
        currentStartTimeInput.value.split(':');
      const [seconds, milliseconds] = fullSeconds.includes('.')
        ? fullSeconds.split('.')
        : [fullSeconds, '000'];
      const currentTime = new Date();
      currentTime.setHours(+hours, +minutes, +seconds, +milliseconds);
      const adjustment =
        BigInt(HUNDRED_MILLISECONDS) * BigInt(isIncrease ? 1 : -1);
      currentStartTimeInput.value = dealKfTime(
        BigInt(currentTime.getTime()) * BigInt(SCALE) + adjustment,
      );
    }
  } else {
    currentStartTimeInput.value = '';
  }
  inputRef.value.focus();
  validateAndUpdateStartTime();
};

const increaseTimestamp = () => {
  modifyTimestamp(true);
};
const decreaseTimestamp = () => {
  modifyTimestamp(false);
};

watch(
  () => currentSession.value,
  (newSession) => {
    if (newSession) {
      channels.value = {};
      frameFilter.value?.resetFilters();
    }
  },
  {
    deep: true,
  },
);

watch(
  () => currentTime.value,
  debounce((newVal, oldVal) => {
    if (newVal === oldVal) return;
    currentStartTimeInput.value = dealKfTime(currentTime.value);
    init();
  }, 100),
);

watch(
  () => firstSplitFramesLoading.value,
  (newIsLoading, oldIsLoading) => {
    if (!newIsLoading && oldIsLoading) {
      handleScrollToTop();
    }
  },
);
let initTimer: NodeJS.Timeout | null = null;

onMounted(() => {
  init();
  nextTick(() => {
    if (initTimer) clearInterval(initTimer);
    initTimer = setInterval(() => {
      if (
        !isLoadingFrames.value &&
        currentFrameList.value.length < DEFAULT_LIST_SIZE
      ) {
        init();
      }
    }, 10000);
  });
});

onBeforeMount(() => {
  if (initTimer) {
    clearInterval(initTimer);
  }
});

const init = debounce(() => {
  console.warn('init');
  if (!currentSession.value) return;
  if (!currentTracer) {
    currentTracer = tracer(
      currentSession.value as KungfuApi.KfLocation,
      readEvent.value,
      writeEvent.value,
      currentSession.value.begin_time,
      currentSession.value.end_time,
    );
  }
  initLoad();
}, 50);

const initLoad = debounce(async () => {
  console.warn('initLoad');
  if (!currentSession.value) return;
  const sessionIdOrigin = currentSession.value.index;
  isLoadingFrames.value && (requestBreakLoadingDataWhile = true);
  firstSplitFramesLoading.value = true;
  // wait for while looping and break while working
  await delayMilliSeconds(0);
  clearSearchState();
  setCurrentFrameList([]);
  await nextTick();
  currentTracer?.seekToTime(currentTime.value);
  await loadFrameData(sessionIdOrigin);
  requestBreakLoadingDataWhile = false;
  firstSplitFramesLoading.value = false;
}, 50);

const loadFrameData = async (currentSessionId: number, loadmore = false) => {
  console.warn('loadFrameData, loadmore', loadmore);
  const drain = async (
    sessionId: number,
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
      currentSession.value &&
      sessionId === currentSession.value.index
    ) {
      tempCount++;
      totalCount++;
      const frame = currentTracer.currentFrame();
      if (!frame) {
        break;
      }

      const msgType = frame.msgType();
      if (
        selectedMsgTypes.value.length > 0 &&
        !selectedMsgTypesMap.value[msgType]
      ) {
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
        currentSession.value,
        sourceDestMap,
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
      currentSession.value &&
      sessionId === currentSession.value.index &&
      !requestBreakLoadingDataWhile &&
      firstSplitFramesLoading.value
    ) {
      firstSplitFramesLoading.value = false;
    }

    if (
      requestBreakLoadingDataWhile ||
      !(currentSession.value && sessionId === currentSession.value.index)
    ) {
      setCurrentFrameList([]);
      return [];
    } else {
      setCurrentFrameList([...currentFrameList.value, ...tempFrames]);
    }

    if (
      !currentTracer.dataAvailable() ||
      currentFrameList.value.length >= DEFAULT_LIST_SIZE ||
      totalCount >= DEFAULT_LIST_SIZE ||
      loadmore
    ) {
      return currentFrameList.value;
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

  isLoadingFrames.value = true;
  return drain(currentSessionId).then((_: KungfuApi.FrameResolved[]) => {
    isLoadingFrames.value = false;
    firstSplitFramesLoading.value = false;
    requestBreakLoadingDataWhile = false;
    setCurrentFrameId(currentFrameList.value[0]?.id);
  });
};

const handleOpenFrameDetail = async ({ row }) => {
  setCurrentFrameId(row.id);
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
    currentSession.value as KungfuApi.KfLocation,
    readEvent.value,
    writeEvent.value,
    currentSession.value?.begin_time as bigint,
    currentSession.value?.end_time as bigint,
  );
  console.warn('on filter');
  initLoad();
};

const dealTagBackgroundColor = (colorStr: string) => {
  if (!colorStr || colorStr === 'default') return '';
  let color = colorMap[colorStr];
  return color;
};

const dealRowClassName = (row) => {
  return row.id === currentFrameId.value ? 'kf-current-table-select' : '';
};

function handleClickRow({ row }) {
  setCurrentFrame(row);
  currentFrameId.value === row.id
    ? setCurrentFrameId('')
    : setCurrentFrameId(row.id);
}
</script>

<style lang="less">
.kf-journal-events__wrap {
  height: 100%;
  width: 100%;
  display: flex;
  flex-direction: column;
  padding: 4px 0 4px 4px;
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
    overflow: hidden;
    position: relative;

    .fade-enter-active,
    .fade-leave-active {
      transition: all 0.3s ease;
    }

    .fade-enter-from,
    .fade-leave-to {
      top: -40px;
    }

    .fade-enter-to,
    .fade-leave-from {
      top: 0;
    }

    .kf-search-in-table__warp {
      position: absolute;
      right: 16px;
      padding: 4px 0;
      display: flex;
      justify-content: flex-end;
      align-items: center;
      font-size: 12px;
      background-color: #1d1d1d;
      z-index: 999;

      .kf-search-in-table__content {
        width: 480px;
        display: flex;
        align-items: center;

        .kf-search-in-table__item {
          margin: 0 4px;
        }

        .ant-input-search {
          margin-left: 0;
          flex: 1;
        }
      }
    }
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

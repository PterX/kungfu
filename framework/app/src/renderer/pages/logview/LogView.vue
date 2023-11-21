<script setup lang="ts">
import { computed, onMounted, ref, watch } from 'vue';
import {
  UpOutlined,
  DownOutlined,
  ReloadOutlined,
} from '@ant-design/icons-vue';

import {
  messagePrompt,
  removeLoadingMask,
  useScrollerTableSearch,
} from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/uiUtils';
import KfDashboard from '@kungfu-trader/kungfu-app/src/renderer/components/public/KfDashboard.vue';
import KfDashboardItem from '@kungfu-trader/kungfu-app/src/renderer/components/public/KfDashboardItem.vue';
import { ensureFileSync, outputFile } from 'fs-extra';
import { shell } from '@electron/remote';
import {
  useLogInit,
  dealLogMessage,
} from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/logUtils';
import VueI18n from '@kungfu-trader/kungfu-js-api/language';

const { t } = VueI18n.global;
const { success, error } = messagePrompt();

defineExpose({
  resetLog,
  getListLength,
});

const props = withDefaults(
  defineProps<{
    logPath: string;
  }>(),
  {
    logPath: '',
  },
);

const DEFAULT_UPDATE_INTERVAL = 150;
const DEFAULT_LINES = 20000;
const IGNORED_INTERVAL_LINES = 10000;
const DEFAULT_MIN_ITEM_SIZE = 36;

let logPath = props.logPath || '';

watch(
  () => props.logPath,
  (newVal) => {
    if (newVal) {
      logPath = props.logPath;
      resetLog();
    }
  },
);

const boardSize = ref<{ width: number; height: number }>({
  width: 0,
  height: 0,
});

const handleChangeBoardSize = ({
  width,
  height,
}: {
  width: number;
  height: number;
}) => {
  boardSize.value.width = width;
  boardSize.value.height = height;
};

const {
  logList,
  scrollToBottomChecked,
  scrollerTableRef,
  isLoading,
  scrollToBottom,
  startTailLog,
  clearLogState,
} = useLogInit(DEFAULT_LINES);

const {
  inputSearchRef,
  searchKeyword,
  currentResultIndex,
  totalResultCount,
  clearSearchState,
  handleToDownSearchResult,
  handleToUpSearchResult,
  getItemHtmlResult,
} = useScrollerTableSearch(
  () => logList.list,
  'id',
  ['message'],
  scrollerTableRef,
);

onMounted(() => {
  removeLoadingMask();
  resetLog();
  scrollerTableRef.value?.$el.addEventListener('scroll', scrollHeader);
});

let timer;
const scrollHeader = (e) => {
  if (timer) clearTimeout(timer);
  if (e?.detail === 'handle') return;

  //数据量大时快速滚动会导致加载dom不准确，需要手动触发一次滚动事件进行渲染
  timer = setTimeout(() => {
    scrollerTableRef.value.$refs.scroller.$_scrollDirty = false;
    scrollerTableRef.value.$refs.scroller.$_lastUpdateScrollPosition -=
      DEFAULT_MIN_ITEM_SIZE;
    const newEvent = new CustomEvent('scroll', { detail: 'handle' });
    scrollerTableRef.value?.$el.dispatchEvent(newEvent);
  }, DEFAULT_UPDATE_INTERVAL * 2);
};

function handleRemoveLog(): Promise<void> {
  ensureFileSync(logPath);
  return outputFile(logPath, '')
    .then(() => {
      success();
      resetLog();
    })
    .catch((err: Error) => {
      error(err.message || t('operation_failed'));
    });
}

function handleOpenFileLocation() {
  return shell.showItemInFolder(logPath);
}

function resetLog() {
  clearLogState();
  clearSearchState();
  startTailLog(logPath);
}

function getListLength() {
  return logList.list.length;
}

const updateInterval = computed(() => {
  if (scrollToBottomChecked.value) {
    return 0;
  } else if (logList.list.length <= IGNORED_INTERVAL_LINES) {
    return 0;
  } else {
    return (
      (DEFAULT_UPDATE_INTERVAL *
        (logList.list.length - IGNORED_INTERVAL_LINES)) /
      (DEFAULT_LINES - IGNORED_INTERVAL_LINES)
    );
  }
});
</script>
<template>
  <div class="default-log-view_warp">
    <a-layout>
      <div class="kf-log-view__warp">
        <KfDashboard @boardSizeChange="handleChangeBoardSize">
          <template #title>
            <slot name="title"></slot>
          </template>
          <template #header>
            <KfDashboardItem>
              <a-button
                v-if="isLoading"
                type="text"
                size="small"
                :loading="isLoading"
                style="pointer-events: none"
              >
                {{ $t('logview.loading_data') }}
              </a-button>
            </KfDashboardItem>
            <slot name="action"></slot>
            <KfDashboardItem>
              <a-checkbox
                v-model:checked="scrollToBottomChecked"
                size="small"
                @change="scrollToBottom"
              >
                {{ $t('logview.scroll_to_bottom') }}
              </a-checkbox>
            </KfDashboardItem>
            <KfDashboardItem>
              <div class="search-in-table__warp">
                <a-input-search
                  ref="inputSearchRef"
                  v-model:value="searchKeyword"
                  class="search-int-table__item"
                  :placeholder="$t('keyword_input')"
                  style="width: 120px"
                />
                <div class="current-to-total search-int-table__item">
                  {{ currentResultIndex }} /
                  {{ totalResultCount }}
                </div>
                <div
                  class="find-up-down search-int-table__item kf-actions__warp"
                >
                  <up-outlined
                    style="font-size: 14px; margin-left: 0px"
                    @click="handleToUpSearchResult"
                  />
                  <down-outlined
                    style="font-size: 14px; margin-left: 8px"
                    @click="handleToDownSearchResult"
                  />
                </div>
              </div>
            </KfDashboardItem>
            <KfDashboardItem>
              <a-button size="small">
                <template #icon>
                  <ReloadOutlined
                    class="kf-hover"
                    style="font-size: 14px"
                    @click="resetLog"
                  />
                </template>
              </a-button>
            </KfDashboardItem>
            <KfDashboardItem>
              <a-button size="small" @click="handleOpenFileLocation">
                {{ $t('folder') }}
              </a-button>
            </KfDashboardItem>
            <KfDashboardItem>
              <a-button size="small" type="primary" @click="handleRemoveLog">
                {{ $t('clean') }}
              </a-button>
            </KfDashboardItem>
          </template>

          <DynamicScroller
            id="kf-log-table"
            ref="scrollerTableRef"
            class="kf-table"
            :items="logList.list"
            :update-interval="updateInterval"
            :min-item-size="DEFAULT_MIN_ITEM_SIZE"
            :simple-array="true"
          >
            <template
              #default="{
                item,
                index,
                active,
              }: {
                item: KungfuApi.KfLogData,
                index: number,
                active: boolean,
              }"
            >
              <DynamicScrollerItem
                :item="item"
                :key="item.id"
                :active="active"
                :size-dependencies="[item.message]"
                :data-index="index"
                :data-active="active"
              >
                <div
                  :id="`kf-log-item-${item.id}`"
                  class="kf-log-line"
                  v-html="dealLogMessage(getItemHtmlResult(item, 'message'))"
                ></div>
              </DynamicScrollerItem>
            </template>
          </DynamicScroller>
        </KfDashboard>
      </div>
    </a-layout>
  </div>
</template>

<style lang="less">
@import '@kungfu-trader/kungfu-app/src/renderer/assets/less/base.less';
@import '@kungfu-trader/kungfu-app/src/renderer/assets/less/public.less';
@import '@kungfu-trader/kungfu-app/src/renderer/assets/less/coverAnt.less';
@import '@kungfu-trader/kungfu-app/src/renderer/assets/less/variables.less';

.default-log-view_warp {
  height: 100%;
  .ant-layout {
    height: 100%;
    background: @component-background;

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
  }

  .kf-log-view__warp {
    height: 100%;
    width: 100%;
    padding: 0 8px 8px 8px;
  }

  .replay_title {
    font-size: 12px;
  }

  .kf-log-line {
    text-align: left;
    font-size: 14px;
    user-select: text;
    padding-bottom: 4px;
    line-height: 1.5;
    word-break: break-all;

    .error {
      color: lighten(@red2-base, 10%);
      font-weight: bold;
    }

    .debug {
      color: @blue-6;
      font-weight: bold;
    }

    .info {
      color: @green2-base;
      font-weight: bold;
    }

    .warning {
      color: @orange-6;
      font-weight: bold;
    }

    .trace {
      color: @cyan-6;
      font-weight: bold;
    }

    .critical {
      color: lighten(@red2-base, 10%);
      font-weight: bold;
    }

    .search-keyword {
      background: fade(@white, 70%);
      color: #000;
      font-weight: normal;

      &.current-search-pointer {
        background: @primary-color;
        color: #fff;
      }
    }
  }
}
</style>

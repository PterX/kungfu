<script setup lang="ts">
import { computed, onMounted, ref } from 'vue';
import {
  UpOutlined,
  DownOutlined,
  ReloadOutlined,
} from '@ant-design/icons-vue';

import {
  messagePrompt,
  removeLoadingMask,
  setHtmlTitle,
  useScrollerTableSearch,
} from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/uiUtils';
import KfDashboard from '@kungfu-trader/kungfu-app/src/renderer/components/public/KfDashboard.vue';
import KfDashboardItem from '@kungfu-trader/kungfu-app/src/renderer/components/public/KfDashboardItem.vue';
import { ensureFileSync, outputFile } from 'fs-extra';
import { shell } from '@electron/remote';
import {
  getLogPath,
  useLogInit,
  dealLogMessage,
} from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/logUtils';
import VueI18n from '@kungfu-trader/kungfu-js-api/language';

const { t } = VueI18n.global;
const { success, error } = messagePrompt();

const LOG_PATH = getLogPath();
setHtmlTitle(LOG_PATH);

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

const DEFAULT_UPDATE_INTERVAL = 150;
const DEFAULT_LINES = 50000;
const IGNORED_INTERAL_LINES = 10000;

const {
  logList,
  scrollToBottomChecked,
  scrollerTableRef,
  isLoading,
  scrollToBottom,
  startTailLog,
  clearLogState,
} = useLogInit(LOG_PATH);

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
    scrollerTableRef.value.$refs.scroller.$_lastUpdateScrollPosition -= 35;
    console.log(
      'scrollerTableRef.value.$refs.$scrollDirty',
      e,
      scrollerTableRef.value.$refs.scroller.$_scrollDirty,
    );
    const newEvent = new CustomEvent('scroll', { detail: 'handle' });
    scrollerTableRef.value?.$el.dispatchEvent(newEvent);
  }, 500);
};

function handleRemoveLog(): Promise<void> {
  ensureFileSync(LOG_PATH);
  return outputFile(LOG_PATH, '')
    .then(() => {
      success();
      resetLog();
    })
    .catch((err: Error) => {
      error(err.message || t('operation_failed'));
    });
}

function handleOpenFileLocation() {
  return shell.showItemInFolder(LOG_PATH);
}

function resetLog() {
  clearLogState();
  clearSearchState();
  startTailLog();
}

const updateIntervalRef = computed(() => {
  return setUpdateInterval(logList.list.length);
});

const setUpdateInterval = (count: number) => {
  if (scrollToBottomChecked.value) {
    return 0;
  } else if (count <= IGNORED_INTERAL_LINES) {
    return 0;
  } else {
    console.log('updateIntervalRef', count, updateIntervalRef.value);
    return (
      (DEFAULT_UPDATE_INTERVAL * (count - IGNORED_INTERAL_LINES)) /
      (DEFAULT_LINES - IGNORED_INTERAL_LINES)
    );
  }
};
</script>
<template>
  <a-layout>
    <div class="kf-log-view__warp">
      <KfDashboard @boardSizeChange="handleChangeBoardSize">
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
              <div class="find-up-down search-int-table__item kf-actions__warp">
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
          :update-interval="updateIntervalRef"
          :min-item-size="36"
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
              :data-active="active"
              :size-dependencies="[item.message]"
              :data-index="index"
            >
              <div
                :id="`kf-log-item-${item.id}`"
                :active="active"
                class="kf-log-line"
                v-html="dealLogMessage(getItemHtmlResult(item, 'message'))"
              ></div>
            </DynamicScrollerItem>
          </template>
        </DynamicScroller>
      </KfDashboard>
    </div>
  </a-layout>
</template>

<style lang="less">
@import '@kungfu-trader/kungfu-app/src/renderer/assets/less/base.less';
@import '@kungfu-trader/kungfu-app/src/renderer/assets/less/public.less';
@import '@kungfu-trader/kungfu-app/src/renderer/assets/less/coverAnt.less';
@import '@kungfu-trader/kungfu-app/src/renderer/assets/less/variables.less';

#app {
  width: 100%;
  height: 100%;
  -webkit-font-smoothing: antialiased;
  -moz-osx-font-smoothing: grayscale;
  text-align: center;

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

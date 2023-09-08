<script setup lang="ts">
import { nextTick, onMounted, ref } from 'vue';
import {
  UpOutlined,
  DownOutlined,
  ReloadOutlined,
} from '@ant-design/icons-vue';

import {
  messagePrompt,
  removeLoadingMask,
  setHtmlTitle,
} from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/uiUtils';
import KfDashboard from '@kungfu-trader/kungfu-app/src/renderer/components/public/KfDashboard.vue';
import KfDashboardItem from '@kungfu-trader/kungfu-app/src/renderer/components/public/KfDashboardItem.vue';
import { ensureFileSync, outputFile } from 'fs-extra';
import { shell } from '@electron/remote';
import { clipboard } from 'electron';
import { platform } from 'os';
import {
  useLogInit,
  useLogSearch,
} from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/logUtils';
import VueI18n from '@kungfu-trader/kungfu-js-api/language';

const { t } = VueI18n.global;
const { success, error } = messagePrompt();

defineExpose({
  resetLog,
});

const props = withDefaults(
  defineProps<{
    logPath: string;
  }>(),
  {
    logPath: '',
  },
);

const LOG_PATH = props.logPath || '';

setHtmlTitle(props.logPath);

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
  scrollToBottom,
  startTailLog,
  clearLogState,
} = useLogInit(LOG_PATH);

const {
  inputSearchRef,
  searchKeyword,
  currentResultPointerIndex,
  totalResultCount,
  clearLogSearchState,
  handleToDownSearchResult,
  handleToUpSearchResult,
} = useLogSearch(logList, scrollerTableRef, boardSize);

onMounted(() => {
  removeLoadingMask();
  resetLog();
});

document.addEventListener('keydown', (e) => {
  const ctrlCmd = platform() === 'darwin' ? e.metaKey : e.ctrlKey;
  if (ctrlCmd && e.key === 'f') {
    searchKeyword.value = clipboard.readText();
    if (inputSearchRef.value) {
      const $inputWrapper = inputSearchRef.value.$el.firstElementChild;
      const $input = $inputWrapper.querySelector('input');
      if ($input) {
        $input.focus();
        nextTick().then(() => {
          $input.select();
        });
      }
    }
  }
});

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
  clearLogSearchState();
  startTailLog();
}
</script>
<template>
  <div class="default-log-view_warp">
    <a-layout>
      <div class="kf-log-view__warp">
        <KfDashboard @boardSizeChange="handleChangeBoardSize">
          <template #title>
            <slot name="replayTitle"></slot>
          </template>
          <template #header>
            <slot name="retry"></slot>
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
                  {{ currentResultPointerIndex }} /
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
                  <ReloadOutlined class="kf-hover" style="font-size: 14px" />
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
                :active="active"
                :size-dependencies="[item.message]"
                :data-index="index"
              >
                <div
                  :id="`kf-log-item-${item.id}`"
                  :active="active"
                  class="kf-log-line"
                  v-html="item.messageForSearch || item.message"
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

<script setup lang="ts">
import { setHtmlTitle } from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/uiUtils';
import { getUrlParams } from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/logUtils';
import Replay from '@kungfu-trader/kungfu-app/src/renderer/pages/replay/Replay.vue';

const params = getUrlParams();

const LOG_PATH = params.logPath || '';
setHtmlTitle(LOG_PATH);

window.onbeforeunload = (e) => {
  e.returnValue = false;
};
</script>
<template>
  <Replay :params="params" :closeImmediately="false" />
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

  .default-log-view_warp {
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
}
</style>

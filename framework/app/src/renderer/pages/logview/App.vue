<script setup lang="ts">
import { setHtmlTitle } from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/uiUtils';
import { getUrlParams } from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/logUtils';
import LogView from '@kungfu-trader/kungfu-app/src/renderer/pages/logview/LogView.vue';

const params = getUrlParams();
console.log(params);

const LOG_PATH = params.logPath || '';
setHtmlTitle(LOG_PATH);
</script>
<template>
  <LogView :log-path="LOG_PATH" />
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
}
</style>

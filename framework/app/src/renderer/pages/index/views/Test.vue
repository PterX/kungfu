<script setup lang="ts">
import { onMounted, onActivated } from 'vue';
import KfRowColIter from '@kungfu-trader/kungfu-app/src/renderer/components/layout/KfRowColIter.vue';

onMounted(() => {
  console.log('Test-mounted', '````````````````````');
});

onActivated(() => {
  console.log('Test-activated', '````````````````````');
});
</script>

<template>
  <div class="kf-basket-trade__warp">
    <KfRowColIter :board-id="0" :closable="false"></KfRowColIter>
  </div>
</template>

<style lang="less">
.kf-basket-trade__warp {
  height: 100%;
  width: 100%;

  .ant-spin.ant-spin-spinning {
    position: unset;
  }

  & > .kf-drag-row__warp {
    height: 100%;
  }
}
</style>

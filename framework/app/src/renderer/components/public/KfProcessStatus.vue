<template>
  <div class="kf-state-status">
    <div v-if="stateStatusData && (stateStatusData.level || 0) !== 0">
      <div
        v-if="isRunning(stateStatusData?.color)"
        class="kf-img-dot kf-state-icon"
      >
        <img :src="runningGif" width="16" height="16" />
      </div>
      <div
        v-else-if="isWaiting(stateStatusData?.color)"
        class="kf-img-dot kf-state-icon"
      >
        <img :src="waitingGif" width="16" height="16" />
      </div>
      <div v-else :class="['kf-dot', stateStatusData?.color || '']"></div>
    </div>
    <div class="kf-state-name">
      {{ +(stateStatusData?.level || 0) === 0 ? '--' : stateStatusData?.name }}
    </div>
  </div>
</template>

<script lang="ts">
import { defineComponent, PropType } from 'vue';
import { getStateStatusData } from '@kungfu-trader/kungfu-js-api/utils/busiUtils';
import { ProcessStatusTypes } from '@kungfu-trader/kungfu-js-api/typings/enums';
import waitingGif from '@kungfu-trader/kungfu-app/src/renderer/assets/imgs/waiting.gif';
import runningGif from '@kungfu-trader/kungfu-app/src/renderer/assets/imgs/running.gif';

export default defineComponent({
  name: 'KfProcessStatus',
  props: {
    statusName: {
      type: String as PropType<ProcessStatusTypes | undefined>,
    },
  },

  data() {
    return {
      waitingGif,
      runningGif,
    };
  },

  computed: {
    stateStatusData(): KungfuApi.KfTradeValueCommonData | undefined {
      return getStateStatusData(this.statusName);
    },
  },

  methods: {
    isWaveStatus(statusColor: KungfuApi.AntInKungfuColorTypes | undefined) {
      if (statusColor === 'kf-color-waiting') return true;
      return false;
    },
    isRunning(statusColor: KungfuApi.AntInKungfuColorTypes | undefined) {
      if (statusColor === 'kf-color-running') return true;
      return false;
    },
    isWaiting(statusColor: KungfuApi.AntInKungfuColorTypes | undefined) {
      if (statusColor === 'kf-color-waiting') return true;
      return false;
    },
  },
});
</script>

<style lang="less">
.kf-state-status {
  display: inline-flex;
  align-items: center;

  .kf-state-icon {
    margin-top: -1px;
  }

  .kf-state-name {
    position: relative;
    top: -1px;
  }
}
</style>

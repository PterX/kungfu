<template>
  <div class="kf-state-status">
    <div
      v-if="stateStatusData && (stateStatusData.level || 0) !== 0"
      :class="['kf-dot', stateStatusData?.color || '',
        isWaveStatus(stateStatusData?.color) ? 'kf-dot-wave' : '',
]"
    ></div>
    <div class="kf-state-name">
      {{ +(stateStatusData?.level || 0) === 0 ? '--' : stateStatusData?.name }}
    </div>
  </div>
</template>

<script lang="ts">
import { defineComponent, PropType } from 'vue';
import { getStateStatusData } from '@kungfu-trader/kungfu-js-api/utils/busiUtils';
import { ProcessStatusTypes } from '@kungfu-trader/kungfu-js-api/typings/enums';

export default defineComponent({
  name: 'KfProcessStatus',
  props: {
    statusName: {
      type: String as PropType<ProcessStatusTypes | undefined>,
    },
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
  },
});
</script>

<style lang="less">
.kf-state-status {
  display: inline-flex;
  align-items: center;

  .kf-state-name {
    position: relative;
    top: -1px;
  }
}
</style>

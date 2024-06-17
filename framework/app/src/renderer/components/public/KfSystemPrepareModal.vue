<script setup lang="ts">
import waitingGif from '@kungfu-trader/kungfu-app/src/renderer/assets/imgs/waiting.gif';
defineProps<{
  visible: boolean;
  status: {
    key: string;
    status: 'done' | 'loading' | undefined;
  }[];
  txt: Record<
    string,
    {
      done: string;
      loading: string;
    }
  >;
  title: string;
}>();

const getContainer = () => document.querySelector('#kf-layout-content');
const isWaiting = (status: 'done' | 'loading' | undefined) => {
  return status === 'loading';
};
</script>
<template>
  <a-modal
    :width="400"
    class="kf-system-prepare-modal"
    :title="title"
    :visible="visible"
    :closable="false"
    :maskClosable="false"
    :footer="null"
    wrapClassName="kf-system-prepare-modal-wrapper"
    :maskStyle="{ position: 'absolute' }"
    :getContainer="getContainer"
  >
    <div class="prepare-item" v-for="item in status" :key="item.key">
      <span v-if="isWaiting(item.status)" class="kf-img-dot">
        <img :src="waitingGif" width="16" height="16" />
      </span>
      <span v-else :class="['kf-dot', 'kf-color-running']"></span>
      <span>{{ item.status ? txt[item.key][item.status] : '' }}</span>
    </div>
  </a-modal>
</template>
<style lang="less">
.kf-system-prepare-modal-wrapper {
  position: absolute !important;
}

.kf-system-prepare-modal {
  .prepare-item {
    margin: 10px 10px 10px 0;
    span {
      margin-right: 10px;
    }
  }
}
</style>

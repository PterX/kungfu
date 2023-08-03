<template>
  <div class="kf-dashboard-item__warp">
    <slot></slot>
  </div>
</template>
<script lang="ts">
import { defineComponent } from 'vue';
export default defineComponent({});
</script>
<style lang="less">
.kf-dashboard-item__warp {
  height: 32px;
  margin-left: 8px;
  white-space: nowrap;
  display: flex;
  align-items: center;
  flex-wrap: wrap;

  &:only-child {
    height: 100%;
  }

  &:first-child {
    margin-left: 0;
  }

  .ant-btn.ant-btn-sm {
    height: 28px;
  }

  .ant-input-search-button {
    height: 28px;
    width: 28px;
  }
}
</style>

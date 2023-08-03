<template>
  <div class="kf-dashboard__warp kf-translateZ">
    <div class="kf-dashboard__header">
      <div v-if="$slots.title" class="title">
        <slot name="title"></slot>
      </div>
      <div v-if="$slots.header" class="header-actions">
        <slot name="header"></slot>
      </div>
    </div>
    <div ref="kfDashboardBody" class="kf-dashboard__body">
      <slot></slot>
    </div>
  </div>
</template>
<script lang="ts">
import { defineComponent, nextTick, onBeforeUnmount, PropType } from 'vue';
import { filter } from 'rxjs';

export default defineComponent({
  name: 'KfDashboard',

  props: {
    title: {
      type: String as PropType<string>,
      default: '',
    },
  },

  mounted() {
    nextTick().then(() => {
      this.$emit('boardSizeChange', this.getBodyWidthHeight());
    });

    if (this.$globalBus) {
      const subscription = this.$globalBus
        .pipe(filter((e: KfEvent.KfBusEvent) => e.tag === 'resize'))
        .subscribe(() => {
          this.$emit('boardSizeChange', this.getBodyWidthHeight());
        });

      onBeforeUnmount(() => {
        subscription.unsubscribe();
      });
    }
  },

  methods: {
    getBodyWidthHeight(): { width: number; height: number } {
      const dashboardBody = this.$refs['kfDashboardBody'] as HTMLElement;
      return {
        width: dashboardBody?.clientWidth || 0,
        height: dashboardBody?.clientHeight || 0,
      };
    },
  },
});
</script>
<style lang="less">
.kf-dashboard__warp {
  height: 100%;
  width: 100%;
  display: flex;
  flex-direction: column;
  padding: 4px 8px;
  box-sizing: border-box;
  overflow: hidden;

  .kf-dashboard__header {
    width: 100%;
    min-height: 32px;
    line-height: 32px;
    margin-bottom: 4px;
    display: flex;
    align-items: flex-start;
    justify-content: space-between;

    .title {
      margin-right: 16px;

      &:only-child {
        margin-right: 0;
      }

      display: flex;
      justify-content: flex-start;
      align-items: center;
      white-space: break-spaces;
      word-break: break-all;

      .name {
        font-size: 14px;
        font-weight: bold;
        color: @white;
        user-select: text;
      }
    }

    .header-actions {
      height: 100%;

      &:only-child {
        width: 100%;
      }

      display: flex;
      justify-content: flex-end;
      align-items: center;
      align-content: flex-start;
      flex-wrap: wrap;
    }
  }

  .kf-dashboard__body {
    width: 100%;
    flex: 1;
    overflow: hidden;

    .ant-table-wrapper {
      height: 100%;
    }
  }
}
</style>

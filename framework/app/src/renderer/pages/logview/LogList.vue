<!-- eslint-disable vue/no-v-html -->
<template>
  <DynamicScroller
    v-bind="$attrs"
    id="kf-log-table"
    ref="scrollerTableRef"
    class="kf-table"
    :items="logList"
    :min-item-size="minItemSize"
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
        :key="item.id"
        :item="item"
        :active="active"
        :size-dependencies="[item.message]"
        :data-index="index"
        :data-active="active"
      >
        <div
          :id="`kf-log-item-${item.id}`"
          class="kf-log-line"
          :style="{ backgroundColor: itemBackgroundColor }"
          v-html="dealLogMessage(itemFormatter(item))"
        ></div>
      </DynamicScrollerItem>
    </template>
  </DynamicScroller>
</template>

<script lang="ts" setup>
import { onMounted, ref } from 'vue';
import { dealLogMessage } from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/logUtils';

const props = withDefaults(
  defineProps<{
    logList: KungfuApi.KfLogData[];
    minItemSize?: number;
    itemBackgroundColor?: string;
    itemFormatter?: (item: KungfuApi.KfLogData) => string;
  }>(),
  {
    minItemSize: 36,
    itemBackgroundColor: '#000',
    itemFormatter: (item: KungfuApi.KfLogData) => item.message,
  },
);

const scrollerTableRef = ref();

let timer;
const scrollHeader = (e) => {
  if (timer) clearTimeout(timer);
  if (e?.detail === 'handle') return;

  //数据量大时快速滚动会导致加载dom不准确，需要手动触发一次滚动事件进行渲染
  timer = setTimeout(() => {
    scrollerTableRef.value.$refs.scroller.$_scrollDirty = false;
    scrollerTableRef.value.$refs.scroller.$_lastUpdateScrollPosition -=
      props.minItemSize;
    const newEvent = new CustomEvent('scroll', { detail: 'handle' });
    scrollerTableRef.value?.$el.dispatchEvent(newEvent);
  }, props.minItemSize * 2);
};

onMounted(() => {
  scrollerTableRef.value?.$el.addEventListener('scroll', scrollHeader);
});

const scrollToBottom = () => {
  scrollerTableRef.value?.scrollToBottom();
};

const scrollToItem = (index: number) => {
  scrollerTableRef.value?.scrollToItem(index);
};

defineExpose({
  scrollToBottom,
  scrollToItem,
});
</script>

<style lang="less">
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
</style>

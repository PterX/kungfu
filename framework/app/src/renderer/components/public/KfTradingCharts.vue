<template>
  <div ref="chartWrapper" class="kf-charts__wrap">
    <div :id="id" class="kf-chart__content"></div>
  </div>
</template>

<script lang="ts" setup>
import { onMounted, ref, watch, onBeforeUnmount } from 'vue';
import * as echarts from 'echarts';

const props = defineProps<{
  option: echarts.EChartsCoreOption;
}>();

defineEmits<{
  (e: 'update:option', option: echarts.EChartsCoreOption): void;
}>();

const id = ref(new Date().getTime().toString());

const chartWrapper = ref<HTMLElement>();
let myChart: echarts.ECharts | null = null;

const initChart = () => {
  const element = document.getElementById(id.value);
  if (element) {
    myChart = echarts.init(element as HTMLElement);
    setOption(props.option);
  }
};

const setOption = (option: echarts.EChartsCoreOption) => {
  if (myChart) {
    myChart.setOption(option);
  }
};

const initResizeEvent = () => {
  if (chartWrapper.value) {
    const ob = new ResizeObserver(handleWrapperResize);
    ob.observe(chartWrapper.value);

    onBeforeUnmount(() => {
      ob.disconnect();
    });
  }
};

watch(
  () => props.option,
  (newOption) => {
    setOption(newOption);
  },
  {
    deep: true,
  },
);

onMounted(() => {
  initChart();
  initResizeEvent();
});

function handleWrapperResize() {
  if (myChart) {
    myChart.resize();
  }
}
</script>

<style lang="less">
.kf-charts__wrap {
  height: 100%;
  width: 100%;

  .kf-chart__content {
    width: 100%;
    height: 100%;
  }
}
</style>

<template>
  <div ref="chartWrapper" class="kf-charts__wrap">
    <div id="main"></div>
  </div>
</template>

<script lang="ts" setup>
import { onMounted, ref, watch } from 'vue';
import * as echarts from 'echarts/core';
import { CandlestickChart, LineChart } from 'echarts/charts';
import {
  TitleComponent,
  TooltipComponent,
  GridComponent,
  LegendComponent,
  DataZoomComponent,
  MarkLineComponent,
  MarkPointComponent,
} from 'echarts/components';
import { UniversalTransition } from 'echarts/features';
import { SVGRenderer } from 'echarts/renderers';

const props = defineProps<{
  option: echarts.EChartsCoreOption;
}>();

defineEmits<{
  (e: 'update:option', option: echarts.EChartsCoreOption): void;
}>();

echarts.use([
  TitleComponent,
  TooltipComponent,
  GridComponent,
  LegendComponent,
  DataZoomComponent,
  MarkLineComponent,
  MarkPointComponent,
  CandlestickChart,
  LineChart,
  SVGRenderer,
  UniversalTransition,
]);

const chartWrapper = ref<HTMLElement>();
const myChart = ref<echarts.ECharts>();

const initChart = () => {
  const element = document.getElementById('main');
  if (element) {
    myChart.value = echarts.init(element as HTMLElement);
    setOption(props.option);
  }
};

const setOption = (option: echarts.EChartsCoreOption) => {
  if (myChart.value) {
    myChart.value.setOption(option);
  }
};

const initResizeEvent = () => {
  if (chartWrapper.value) {
    chartWrapper.value.addEventListener('resize', handleWrapperResize);
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
  if (myChart.value) {
    myChart.value.resize();
  }
}
</script>

<style lang="less">
.kf-charts__wrap {
  height: 100%;
  width: 100%;

  #main {
    width: 100%;
    height: 100%;
  }
}
</style>

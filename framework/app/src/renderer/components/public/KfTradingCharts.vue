<template>
  <div ref="chartWrapper" class="kf-charts__wrap">
    <div :id="id" :ref="chart" class="kf-chart__content"></div>
  </div>
</template>

<script lang="ts" setup>
import {
  onMounted,
  ref,
  watch,
  onBeforeUnmount,
  getCurrentInstance,
} from 'vue';
import * as echarts from 'echarts';
import { nextTick } from 'vue';

const props = defineProps<{
  id?: string;
  option: echarts.EChartsCoreOption;
}>();

defineEmits<{
  (e: 'update:option', option: echarts.EChartsOption): void;
}>();

const app = getCurrentInstance();
const id = ref(props.id || app?.uid.toString() || Date.now().toString());
const chart = ref();
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
  if (app && !id.value) {
    id.value = app.uid.toString();
  }

  nextTick(() => {
    initChart();
    initResizeEvent();
  });
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
  margin-bottom: 200px;

  .kf-chart__content {
    width: 100%;
    height: 100%;
  }
}
</style>

<template>
  <div class="kf-visual-container">
    <div class="kf-visual-item">
      <KfTradingCharts v-model:option="option1"></KfTradingCharts>
    </div>
    <div class="kf-visual-item">
      <KfTradingCharts v-model:option="option2"></KfTradingCharts>
    </div>
    <div class="kf-visual-item">
      <KfTradingCharts v-model:option="option3"></KfTradingCharts>
    </div>
  </div>
</template>

<script lang="ts" setup>
import dayjs from 'dayjs';
import { reactive } from 'vue';
import KfTradingCharts from '../../../components/public/KfTradingCharts.vue';
// import { useDealJournalDatas } from '../utils';

// useDealJournalDatas();
const upColor = '#ec0000';
const upBorderColor = '#8A0000';
const downColor = '#00da3c';
const downBorderColor = '#008F28';
const dataCount = 200;
const markPoint = {
  label: {
    formatter: function (param) {
      return param;
    },
  },
  data: [
    {
      name: 'Mark',
      coord: ['2022/08/09\n08:09:10', 4000],
      value: ['Trade', 'Order'][Math.floor(Math.random() * 2)],
      itemStyle: {
        color: 'rgb(41,60,85)',
      },
    },
  ],
};
const option1 = reactive(getOption());
const option2 = reactive(getOption());
const option3 = reactive(getOption());

function getOption() {
  const data = generateOHLC(dataCount);
  const option = {
    dataset: {
      source: data,
    },
    tooltip: {
      trigger: 'axis',
      axisPointer: {
        type: 'line',
      },
    },
    toolbox: {
      feature: {
        dataZoom: {
          yAxisIndex: false,
        },
      },
    },
    grid: [
      {
        left: '5%',
        right: '5%',
        bottom: 50,
      },
      {
        left: '5%',
        right: '5%',
        height: 80,
        bottom: 80,
      },
    ],
    xAxis: [
      {
        type: 'category',
        boundaryGap: false,
        // inverse: true,
        axisLine: { onZero: false },
        splitLine: { show: false },
        min: 'dataMin',
        max: 'dataMax',
      },
      {
        type: 'category',
        gridIndex: 1,
        boundaryGap: false,
        axisLine: { onZero: false },
        axisTick: { show: false },
        splitLine: { show: false },
        axisLabel: { show: false },
        min: 'dataMin',
        max: 'dataMax',
      },
    ],
    yAxis: [
      {
        scale: true,
        splitArea: {
          show: true,
        },
      },
      {
        scale: true,
        gridIndex: 1,
        splitNumber: 2,
        axisLabel: { show: false },
        axisLine: { show: false },
        axisTick: { show: false },
        splitLine: { show: false },
      },
    ],
    series: [
      {
        type: 'candlestick',
        itemStyle: {
          color: upColor,
          color0: downColor,
          borderColor: upBorderColor,
          borderColor0: downBorderColor,
        },
        encode: {
          x: 0,
          y: [1, 4, 3, 2],
        },
        markPoint,
        // {
        //   name: 'Volumn',
        //   type: 'bar',
        //   xAxisIndex: 1,
        //   yAxisIndex: 1,
        //   itemStyle: {
        //     color: '#7fbe9e',
        //   },
        //   large: true,
        //   encode: {
        //     x: 0,
        //     y: 5,
        //   },
      },
    ],
  };

  return option;
}

function generateOHLC(count) {
  let data: any[] = [];
  let xValue = +new Date(2022, 7, 22);
  let baseValue = Math.random() * 12000;
  let minute = 60 * 1000;
  let boxVals = new Array(4);
  let dayRange = 12;
  for (let i = 0; i < count; i++) {
    baseValue = baseValue + Math.random() * 20 - 10;
    for (let j = 0; j < 4; j++) {
      boxVals[j] = (Math.random() - 0.5) * dayRange + baseValue;
    }
    boxVals.sort();
    let openIdx = Math.round(Math.random() * 3);
    let closeIdx = Math.round(Math.random() * 2);
    if (closeIdx === openIdx) {
      closeIdx++;
    }
    let volumn = boxVals[3] * (1000 + Math.random() * 500);
    // ['open', 'close', 'lowest', 'highest', 'volumn']
    // [1, 4, 3, 2]
    data[i] = [
      dayjs((xValue += minute)).format('YYYY-MM-DD\nHH:mm:ss'),
      +boxVals[openIdx].toFixed(2),
      +boxVals[3].toFixed(2),
      +boxVals[0].toFixed(2),
      +boxVals[closeIdx].toFixed(2),
      +volumn.toFixed(0),
      getSign(data, i, +boxVals[openIdx], +boxVals[closeIdx], 4), // sign
    ];
  }
  return data;
  function getSign(data, dataIndex, openVal, closeVal, closeDimIdx) {
    var sign;
    if (openVal > closeVal) {
      sign = -1;
    } else if (openVal < closeVal) {
      sign = 1;
    } else {
      sign =
        dataIndex > 0
          ? // If close === open, compare with close of last record
            data[dataIndex - 1][closeDimIdx] <= closeVal
            ? 1
            : -1
          : // No record of previous, set to be positive
            1;
    }
    return sign;
  }
}
</script>

<style lang="less">
.kf-visual-container {
  height: 100%;
  width: 100%;
  display: flex;
  flex-direction: column;
  justify-content: space-around;
  align-items: center;

  .kf-visual-item {
    width: 100%;
    height: 32%;
  }
}
</style>

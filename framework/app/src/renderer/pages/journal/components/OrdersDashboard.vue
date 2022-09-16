<template>
  <div class="kf-visual-container">
    <div class="kf-visual-item">
      <KfTradingCharts v-model:option="option"></KfTradingCharts>
    </div>
    <div class="kf-visual-item">
      <KfTradingCharts v-model:option="option"></KfTradingCharts>
    </div>
    <div class="kf-visual-item">
      <KfTradingCharts v-model:option="option"></KfTradingCharts>
    </div>
  </div>
</template>

<script lang="ts" setup>
import { reactive } from 'vue';
import KfTradingCharts from '../../../components/public/KfTradingCharts.vue';
// import { useDealJournalDatas } from '../utils';

// useDealJournalDatas();

const option = reactive({
  xAxis: {
    data: ['2017-10-24', '2017-10-25', '2017-10-26', '2017-10-27'],
  },
  yAxis: {},
  series: [
    {
      type: 'candlestick',
      data: [
        [20, 34, 10, 38],
        [40, 35, 30, 50],
        [31, 38, 33, 44],
        [38, 15, 5, 42],
      ],
    },
  ],
});
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

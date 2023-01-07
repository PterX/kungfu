<template>
  <div class="kf-visual-container">
    <div
      v-for="key in Object.keys(allOptions)"
      :key="key"
      class="kf-visual-item"
    >
      <KfTradingCharts v-model:option="allOptions[key]"></KfTradingCharts>
    </div>
  </div>
</template>

<script lang="ts" setup>
import { dealKfTime } from '@kungfu-trader/kungfu-js-api/kungfu';
import { dealKfPrice } from '@kungfu-trader/kungfu-js-api/utils/busiUtils';
import { computed } from 'vue';
import KfTradingCharts from '../../../components/public/KfTradingCharts.vue';
import { useDealJournalDatas } from '../utils';

const { allTradingDatas } = useDealJournalDatas();

const allOptions = computed(() => {
  return Object.keys(allTradingDatas.value).reduce((options, key) => {
    const { quotes, trades, orders } = allTradingDatas.value[key];

    const markPoint = getMarkPoint({ Trade: trades, Order: orders });

    const data = quotes.map((quote) => [
      dealKfTime(BigInt(quote.data_time)),
      dealKfPrice(quote.open_price),
      dealKfPrice(quote.close_price),
      dealKfPrice(quote.high_price),
      dealKfPrice(quote.low_price),
    ]);
    return { ...options, [key]: getOption(key, data, markPoint) };
    // eslint-disable-next-line @typescript-eslint/no-explicit-any
  }, {} as Record<string, any>);
});

const upColor = '#ec0000';
const upBorderColor = '#8A0000';
const downColor = '#00da3c';
const downBorderColor = '#008F28';

const getMarkPoint = (data: {
  Trade: KungfuApi.Trade[];
  Order: KungfuApi.Order[];
}) => {
  const markPoint = {
    label: {
      formatter: function (param) {
        return param;
      },
    },
    data: Object.keys(data)
      .map((type) => {
        const timeKey = type === 'Order' ? 'update_time' : 'trade_time';
        const priceKey = type === 'Order' ? 'limit_price' : 'price';

        return data[type].map((item) => ({
          name: 'Mark',
          coord: [
            dealKfTime(BigInt(item[timeKey])),
            dealKfPrice(item[priceKey]),
          ],
          value: type,
          itemStyle: {
            color: 'rgb(41,60,85)',
          },
        }));
      })
      .flat(),
  };

  return markPoint;
};

function getOption(title: string, data, markPoint) {
  const option = {
    title: {
      text: title,
    },
    dataset: {
      source: data,
    },
    tooltip: {
      trigger: 'axis',
      axisPointer: {
        type: 'line',
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
        height: '90%',
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
        dimensions: ['date', 'open', 'close', 'highest', 'lowest'],
        encode: {
          x: 'date',
          y: ['open', 'close', 'highest', 'lowest'],
        },
        markPoint,
      },
    ],
  };

  return option;
}
</script>

<style lang="less">
.kf-visual-container {
  height: 100%;
  width: 100%;
  overflow-y: scroll;

  .kf-visual-item {
    width: 100%;
    height: 400px;
  }
}
</style>

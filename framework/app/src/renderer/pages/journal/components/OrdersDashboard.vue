<template>
  <div class="kf-visual-container">
    <div class="kf-visual-search">
      <a-input-search
        v-model:value="searchKey"
        class="search-input"
        type="text"
        :placeholder="$t('journalConfig.search_instrument_id')"
        style="width: 120px"
        @search="scrollToKey"
      />
    </div>

    <div ref="chartsContainer" class="kf-visual-charts-container">
      <div
        v-for="key in Object.keys(allOptions)"
        :key="key"
        :ref="(el) => (chartRefs[key] = el)"
        class="kf-visual-item"
      >
        <KfTradingCharts v-model:option="allOptions[key]"></KfTradingCharts>
      </div>
    </div>
  </div>
</template>

<script lang="ts" setup>
import { dealKfTime } from '@kungfu-trader/kungfu-js-api/kungfu';
import { dealKfPrice } from '@kungfu-trader/kungfu-js-api/utils/busiUtils';
import { computed, reactive, ref, watch } from 'vue';
import KfTradingCharts from '../../../components/public/KfTradingCharts.vue';
import { useDealJournalDatas } from '../utils';
import VueI18n from '@kungfu-trader/kungfu-js-api/language';

const { t } = VueI18n.global;
const props = withDefaults(
  defineProps<{
    currentSession: KungfuApi.SessionResolved | null;
  }>(),
  {},
);

const searchKey = ref<string>('');
const chartRefs: { [key: string]: HTMLElement } = reactive({});
const chartsContainer = ref<HTMLElement | null>(null);

const scrollToKey = () => {
  if (!searchKey.value) {
    alert(t('journalConfig.please_input_instrument_id)'));
    return;
  }

  const target = chartRefs[searchKey.value];

  if (!target) {
    alert(t('journalConfig.undefined_instrument_id'));
    return;
  }

  const targetPosition =
    target.offsetTop - (chartsContainer.value?.offsetTop || 0);
  chartsContainer.value!.scrollTop = targetPosition;
};

let { allTradingDatas, mdQuotoDatas } = useDealJournalDatas();

const previousAllTradingDatas = new Map();
let oldOptionData: string[] = [];
const allOptions = computed(() => {
  const resolvedOptions = Object.keys(allTradingDatas.value).reduce(
    (options, key) => {
      let { quotes, trades, orders } = allTradingDatas.value[key];
      if (quotes.length === 0 && mdQuotoDatas.value[key]) {
        quotes = mdQuotoDatas.value[key].mdQuotes;
      }
      if (quotes.length === 0) {
        return { ...options };
      }
      const prevData = previousAllTradingDatas.get(key) || {
        quotes: [],
        trades: [],
        orders: [],
      };

      const quotesDiff = quotes.length - prevData.quotes.length;
      const newQuotes = quotesDiff > 0 ? quotes.slice(-quotesDiff) : quotes;

      const tradesDiff = trades.length - prevData.trades.length;
      const newTrades = tradesDiff > 0 ? trades.slice(-tradesDiff) : trades;

      const ordersDiff = orders.length - prevData.orders.length;
      const newOrders = ordersDiff > 0 ? orders.slice(-ordersDiff) : orders;

      const timestampMap = new Map();
      let timeTemp: string[] = [];
      let timeTemp2: string[] = [];
      quotes.forEach((item) => {
        if (item) {
          const timestampKey = 'data_time';
          const time = dealKfTime(BigInt(item[timestampKey]));
          timeTemp2.push(time);
        }
      });
      const processData = (data, type) => {
        data.forEach((item) => {
          if (!item) return;
          const timestampKey =
            type === 'quote'
              ? 'data_time'
              : type === 'trade'
              ? 'trade_time'
              : 'update_time';
          const priceKey =
            type === 'quote'
              ? 'last_price'
              : type === 'trade'
              ? 'price'
              : 'limit_price';
          if (
            item[timestampKey] !== undefined &&
            item[priceKey] !== undefined
          ) {
            const timestamp = dealKfTime(BigInt(item[timestampKey]));
            timeTemp.push(timestamp);
            const timestampInSeconds = timestamp.slice(0, -4);
            const price = dealKfPrice(item[priceKey]);
            const key = `${timestampInSeconds}`;
            if (timestampMap.has(key)) {
              const currentData = timestampMap.get(key);
              if (currentData.length === 2 && type !== 'quote') {
                currentData.push(price);
                currentData.push(type);
              } else if (type !== 'quote') {
                currentData[2] = price;
                currentData[3] = `${currentData[3]}And${type}`;
              }
            } else if (type === 'quote') {
              timestampMap.set(key, [timestampInSeconds, price]);
            }
          }
        });
      };

      processData(newQuotes, 'quote');
      processData(newTrades, 'trade');
      processData(newOrders, 'order');

      const allData = Array.from(timestampMap.values());
      allData.sort((a, b) => {
        return a[0] > b[0] ? 1 : -1;
      });
      let newData: string[] = [];
      let reset = false;
      if (allData.length > 0) {
        if (
          oldOptionData.length === 0 ||
          allData.length < oldOptionData.length ||
          allData[0][0] !== oldOptionData[0][0] ||
          allData[allData.length - 1][0] <
            oldOptionData[oldOptionData.length - 1][0]
        ) {
          oldOptionData = allData;
          reset = true;
        } else if (allData.length >= oldOptionData.length) {
          newData = allData.slice(oldOptionData.length);
        }
      } else {
        newData = [];
      }

      return {
        ...options,
        [key]: getOption(key, oldOptionData, newData, reset),
      };
    },
    {} as Record<string, any>,
  );

  // 更新 previousAllTradingDatas
  previousAllTradingDatas.clear();
  for (const key in allTradingDatas.value) {
    previousAllTradingDatas.set(key, allTradingDatas.value[key]);
  }

  return resolvedOptions;
});

watch(
  () => props.currentSession,
  (newSession, oldSession) => {
    if (newSession && newSession !== oldSession) {
      // let { allTradingDatas: tradingDatas } = useDealJournalDatas();
      // allTradingDatas = tradingDatas;
      previousAllTradingDatas.clear();
    }
  },
);

function getXAxisData(oldData, newData) {
  if (newData.length <= 0) {
    oldxAxis = oldData.map((item) => item[0].split('.')[0]);
    return oldxAxis;
  }
  return [...oldxAxis, ...newData.map((item) => item[0].split('.')[0])];
}
function getYAxisData(oldData, newData) {
  if (newData.length <= 0) {
    oldyAxis = oldData.map((item) => item[1]);
    return oldyAxis;
  }
  return [...oldyAxis, ...newData.map((item) => item[1])];
}
function getMarkPointData(oldData, newData) {
  if (newData.length <= 0) {
    oldMarkPointData = oldData
      .map((item, index) => ({
        name: item[2],
        value: item[3] ? item[1] : undefined,
        xAxis: index,
        yAxis: item[3] ? item[2] : item[1],
        itemStyle: {
          color:
            item[3] === 'trade' || item[item.length - 1] === 'tradeAndorder'
              ? 'rgb(191, 191, 61)'
              : item[item.length - 1] === 'order'
              ? 'rgb(104, 187, 196)'
              : null,
        },
      }))
      .filter((item) => item.value !== undefined);
    return oldMarkPointData;
  }
  return [
    ...oldMarkPointData,
    ...newData
      .map((item, index) => ({
        name: item[2],
        value: item[3] ? item[1] : undefined,
        xAxis: index + oldMarkPointData.length,
        yAxis: item[3] ? item[2] : item[1],
        itemStyle: {
          color:
            item[3] === 'trade' || item[item.length - 1] === 'tradeAndorder'
              ? 'rgb(191, 191, 61)'
              : item[item.length - 1] === 'order'
              ? 'rgb(104, 187, 196)'
              : null,
        },
      }))
      .filter((item) => item.value !== undefined),
  ];
}

let oldxAxis = [];
let oldyAxis = [];
let oldMarkPointData = [];
function getOption(
  title: string,
  oldData: string[],
  newData: string[],
  reset: boolean,
) {
  if (newData.length > 0) {
    oldOptionData = oldOptionData.concat(newData);
  }
  if (reset) {
    oldxAxis = [];
    oldyAxis = [];
    oldMarkPointData = [];
  }
  if (oldData.length === 0 && newData.length === 0) {
    return {};
  }

  const xAxis = getXAxisData(oldData, newData);
  const yAxis = getYAxisData(oldData, newData);
  const yAxisMin = Math.floor(Math.min(...yAxis) / 10) * 10;
  const yAxisMax = Math.ceil(Math.max(...yAxis) / 10) * 10;
  const markPointData = getMarkPointData(oldData, newData);
  const option = {
    title: {
      text: title,
      left: 'center',
    },
    tooltip: {
      trigger: 'axis',
    },
    legend: {
      orient: 'vertical',
      right: 'right',
      top: 'top',
      data: [
        t('journalConfig.stock_price'),
        t('journalConfig.trade_node'),
        t('journalConfig.order_node'),
      ],
    },
    dataZoom: [
      {
        type: 'slider',
        xAxisIndex: 0,
        start: 0,
        end: (100 * 100) / xAxis.length,
      },
    ],
    xAxis: {
      type: 'category',
      data: xAxis,
      boundaryGap: false,
    },
    yAxis: {
      type: 'value',
      min: yAxisMin,
      max: yAxisMax,
      Interval: (yAxisMax - yAxisMin) / 4,
      minInterval: 1,
      axisLabel: {
        formatter: (value) => {
          if (value === yAxisMin) {
            return `${value}`;
          } else {
            return value;
          }
        },
      },
    },
    series: [
      {
        name: t('journalConfig.stock_price'),
        type: 'line',
        smooth: true,
        data: yAxis,
        markPoint: {
          symbol: 'circle',
          symbolSize: 8,
          data: markPointData,
          label: {
            show: true,
            formatter: '{b}',
            position: 'top',
            fontWeight: 'bold',
            color: 'red',
          },
        },
        itemStyle: {
          color: 'rgb(243, 148, 35)',
        },
      },
      {
        name: t('journalConfig.trade_node'),
        type: 'line',
        data: [],
        markPoint: {
          symbol: 'circle',
          symbolSize: 1,
          data: markPointData.filter(
            (item) => item.name === 'trade' || item.name === 'orderAndTrade',
          ),
          label: {
            show: true,
            formatter: '{b}',
            position: 'top',
            fontWeight: 'bold',
            color: 'rgb(191, 191, 61)',
          },
        },
        itemStyle: {
          color: 'rgb(191, 191, 61)',
        },
      },
      {
        name: t('journalConfig.order_node'),
        type: 'line',
        data: [],
        markPoint: {
          symbol: 'circle',
          symbolSize: 1,
          data: markPointData.filter((item) => item.name === 'order'),
          label: {
            show: true,
            formatter: '{b}',
            position: 'top',
            fontWeight: 'bold',
            color: 'rgb(104, 187, 196)',
          },
        },
        itemStyle: {
          color: 'rgb(104, 187, 196)',
        },
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
  margin-bottom: 20px;

  .kf-visual-item {
    width: 100%;
    height: 400px;
  }
}
.kf-visual-charts-container {
  /* 设置容器的高度和溢出滚动 */
  width: 100%;
  height: 100%; /* 请根据实际需求调整高度 */
  overflow-y: auto;
  margin-bottom: 20px;
}

.kf-visual-search {
  display: flex;
  justify-content: flex-end; /* 让搜索框居右 */
  margin-right: 10px;
}

.search-input {
  color: black; /* 设置搜索框字体颜色为黑色 */
}

button {
  color: black;
}
</style>

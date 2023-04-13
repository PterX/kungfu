<template>
  <div class="kf-visual-container">
    <!-- 搜索框和搜索按钮 -->
    <div class="kf-visual-search">
      <!-- <input
       

        
        placeholder="输入 key"
      />
      <button>{{ t('journalConfig.export') }}</button> -->
      <a-input-search
        v-model:value="searchKey"
        class="search-input"
        type="text"
        :placeholder="$t('journalConfig.search_instrument_id')"
        style="width: 120px"
        @search="scrollToKey"
      />
    </div>

    <!-- 图表容器 -->
    <div ref="chartsContainer" class="kf-visual-charts-container">
      <!-- 图表位置 -->
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
import { computed, reactive, ref, watch, watchEffect } from 'vue';
import { assemble } from '@kungfu-trader/kungfu-js-api/kungfu';
import KfTradingCharts from '../../../components/public/KfTradingCharts.vue';
import { useDealJournalDatas } from '../utils';
import { SessionStatusEnum } from '@kungfu-trader/kungfu-js-api/typings/enums';
import { dealFrame } from '../utils';
// import {  FiltersEnum } from '../utils/filterUtils';
import { useJournalStore } from '../store/journalStore';
import VueI18n from '@kungfu-trader/kungfu-js-api/language';

const { t } = VueI18n.global;
const props = withDefaults(
  defineProps<{
    sessions: KungfuApi.SessionResolved[];
    currentSession: KungfuApi.SessionResolved | null;
    mdSession: KungfuApi.SessionResolved | null;
    currentTimeRangeData: {
      range: [bigint, bigint];
      reload: boolean;
    };
  }>(),
  {},
);

let mdSessionBeginTime = props.mdSession ? props.mdSession.begin_time : 0n;
const searchKey = ref<string>(''); // 搜索框绑定的数据
const chartRefs: { [key: string]: HTMLElement } = reactive({}); // 用于存储图表 DOM 元素的引用
const chartsContainer = ref<HTMLElement | null>(null); // 图表容器的引用

const scrollToKey = () => {
  if (!searchKey.value) {
    alert('请输入 key');
    return;
  }

  const target = chartRefs[searchKey.value];

  if (!target) {
    alert('找不到对应的图表');
    return;
  }

  const targetPosition =
    target.offsetTop - (chartsContainer.value?.offsetTop || 0);
  chartsContainer.value!.scrollTop = targetPosition;
};

// watch(
//   () => props.mdSession,
//   (newSession, oldSession) => {
//     if (newSession && newSession !== oldSession) {
//       console.log('loadmdSession>>>', newSession);
//       loadFrameData(newSession, newSession.begin_time, newSession.end_time);
//     }
//   },
// );
watch(
  () => props.currentSession,
  (newSession, oldSession) => {
    if (newSession && newSession !== oldSession) {
      mdSessionBeginTime = changeMdSessionSrartTime(newSession);
      loadFrameData(
        props.mdSession?.index as number,
        mdSessionBeginTime,
        props.mdSession?.end_time as bigint,
        newSession.index === props.mdSession?.index,
      );
    }
  },
);
const changeMdSessionSrartTime = (session: KungfuApi.SessionResolved) => {
  //与session的session_id_resolved相同的session中，begin_time最小的那个
  const minBeginTime = props.sessions
    .filter((item) => item.session_id_resolved === session.session_id_resolved)
    .reduce((pre, cur) => {
      return pre.begin_time < cur.begin_time ? pre : cur;
    }).begin_time;
  return minBeginTime;
};
const journalStore = useJournalStore();
const EVERY_COUNT = 20;
const LIMIT_COUNT = 1000;
let journalReader: KungfuApi.AssembleReader | null = null;
// const framesMap = shallowRef<Record<string, KungfuApi.FrameResolved>>({});
const loadFrameData = (
  session: KungfuApi.SessionResolved | number,
  startTime: bigint,
  endTime: bigint,
  checking = false,
) => {
  if (!session) return;
  const sessionId = typeof session === 'number' ? session : session?.index;

  if (!checking) {
    if (sessionId === SessionStatusEnum.Running) {
      //todo: 后端需要支持实时读取
      journalReader = assemble.getReader(sessionId, startTime);
    } else {
      journalReader = assemble.getReader(sessionId, startTime, endTime);
    }
  }

  let total = 0;
  const curFramesMap = {};
  return new Promise<KungfuApi.FrameResolved[]>((resolve, _) => {
    const runner = () => {
      setTimeout(() => {
        if (!journalReader) return resolve([]);
        let count = 0;
        journalReader.run((frame) => {
          if (frame) {
            const curFrameData: KungfuApi.Frame = {
              id: total,
              dataLength: frame.dataLength(),
              genTime: frame.genTime(),
              triggerTime: frame.triggerTime(),
              msgType: frame.msgType(),
              stringMsgType: frame.stringMsgType(),
              source: frame.source(),
              dest: frame.dest(),
              data: frame.data(),
              destName: frame.destName(),
              sourceName: frame.sourceName(),
            };
            const curFrameDataResolved = dealFrame(curFrameData);

            curFramesMap[curFrameDataResolved.id] = curFrameDataResolved;

            ++total;
            ++count;
          }
        }, EVERY_COUNT);

        if (count < EVERY_COUNT || total >= LIMIT_COUNT) {
          resolve(Object.values(curFramesMap));
        } else {
          runner();
        }
      });
    };

    runner();
  }).then((res) => {
    if (checking) {
      journalStore.setMdSessionFrames(res, false);
    } else {
      journalStore.setMdSessionFrames(res, true);
    }
    if (total >= LIMIT_COUNT) loadFrameData(session, startTime, endTime, true);
  });
};

const { allTradingDatas, mdQuotoDatas } = useDealJournalDatas();

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

      // 处理 quotes
      const quotesDiff = quotes.length - prevData.quotes.length;
      const newQuotes = quotesDiff > 0 ? quotes.slice(-quotesDiff) : quotes;

      // 处理 trades
      const tradesDiff = trades.length - prevData.trades.length;
      const newTrades = tradesDiff > 0 ? trades.slice(-tradesDiff) : trades;

      // 处理 orders
      const ordersDiff = orders.length - prevData.orders.length;
      const newOrders = ordersDiff > 0 ? orders.slice(-ordersDiff) : orders;

      const timestampMap = new Map();
      let timetemp: string[] = [];
      let timetemp2: string[] = [];
      //将quotes所有的时间戳格式化后放到一个数组里面
      quotes.forEach((item) => {
        if (item) {
          const timestampKey = 'data_time';
          const b = dealKfTime(BigInt(item[timestampKey]));
          timetemp2.push(b);
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
            timetemp.push(timestamp);
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

      // 将 Map 对象转换为数组
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

      console.log('data;quotes;trades;orders', {
        allData,
        timeTemp: timetemp.sort((a, b) => {
          return a > b ? 1 : -1;
        }),
        timetemp2: timetemp2.sort((a, b) => {
          return a > b ? 1 : -1;
        }),
        oldnew: { oldOptionData, newData },
        allTradingDatas,
        ll: { quotes, trades, orders },
        new: {
          newQuotes,
          newTrades,
          newOrders,
        },
      });
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
  console.log(
    'previousAllTradingDatas',
    previousAllTradingDatas,
    resolvedOptions,
  );

  return resolvedOptions;
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
  // console.log('markPoint',markPoint);
  return markPoint;
};

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
    // ...其他配置
    dataZoom: [
      {
        type: 'slider',
        xAxisIndex: 0,
        start: 0,
        end: (100 * 100) / xAxis.length, // 调整 end 值，使初始展示的数据数量约为100条
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
            fontWeight: 'bold', // 设置标签字体加粗
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
            position: 'top', // 设置 markPoint 标签位置
            fontWeight: 'bold', // 设置标签字体加粗
            color: 'rgb(191, 191, 61)', // 设置标签字体颜色
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
            position: 'top', // 设置 markPoint 标签位置
            fontWeight: 'bold', // 设置标签字体加粗
            color: 'rgb(104, 187, 196)', // 设置标签字体颜色
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
watchEffect(() => {
  console.log(
    'allOptions;allTradingDatas;mdQ>>>',
    allOptions.value,
    allTradingDatas.value,
    mdQuotoDatas.value,
    Object.keys(allOptions),
  );
});
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
  color: black; /* 设置搜索按钮字体颜色为黑色 */
}
</style>

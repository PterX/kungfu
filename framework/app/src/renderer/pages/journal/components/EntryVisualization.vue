<template>
  <div class="kf-visualization_wrap">
    <div class="kf-strategy_wrap">
      <a-table
        class="kf-ant-table"
        :columns="columns"
        :data-source="strategyData"
        size="small"
        :pagination="false"
        :row-class-name="dealRowClassName"
        :custom-row="customRow"
        :scroll="{ y: dashboardBodyHeight - 4 }"
        :empty-text="$t('empty_text')"
      >
        <template
          #bodyCell="{
            column,
            record,
          }: {
            column: AntTableColumn,
            record: KungfuApi.SessionResolved,
          }"
        >
          <template v-if="column.dataIndex === 'status'">
            <span
              :title="SessionStatus[record[column.dataIndex]].name"
              :style="{
                backgroundColor: SessionStatus[record[column.dataIndex]].color,
                width: '12px',
                height: '12px',
                margin: '0 auto',
                borderRadius: '50%',
                display: 'inline-block',
              }"
            ></span>
          </template>
        </template>
      </a-table>
    </div>
    <div class="kf-instrument_wrap">
      <div class="search-input">
        <KfDashboardItem>
          <a-input-search
            v-model:value="searchInstrument"
            :placeholder="$t('journalConfig.search_instrument')"
            @change="handleInputChange"
          />
        </KfDashboardItem>
      </div>
      <div class="instrument-list">
        <template v-if="instrumentList.length > 0">
          <div
            v-for="item in instrumentList"
            :key="item"
            :class="{
              'instrument-item_wrap': true,
              'color-default': true,
              'selected-status': selectedInstrument.includes(item),
            }"
            @click="getCurInstrument(item)"
          >
            <span>{{ item }}</span>
          </div>
        </template>

        <a-empty v-else class="kf-chart_content" :image="simpleImage"></a-empty>
      </div>
    </div>
    <div ref="chartWrapper" class="kf-chart_wrap">
      <a-input-search
        v-show="instrumentList.length > 0"
        v-model:value="searchOrderId"
        class="chart-search-order-id"
        :placeholder="$t('journalConfig.search_order_id')"
        @search="handleSearchOrderId"
      />
      <div
        v-show="instrumentList.length > 0"
        id="strategyChart"
        class="kf-chart_content"
      ></div>
      <a-empty
        v-show="instrumentList.length === 0"
        class="kf-chart_content"
        :image="simpleImage"
      ></a-empty>
    </div>
    <a-spin
      class="kf-journal-spin"
      :spinning="isLoadingFrames"
      :tip="$t('journalConfig.loading_journal')"
    />
  </div>
</template>

<script setup lang="ts">
import { storeToRefs } from 'pinia';
import { useJournalStore } from '../store/journalStore';
import {
  getChartOption,
  getStrategyColumns,
  SeriesData,
  SessionStatus,
} from '../config';
import {
  computed,
  nextTick,
  onBeforeUnmount,
  onMounted,
  ref,
  watch,
} from 'vue';
import fse from 'fs-extra';
import path from 'path';
import { Empty } from 'ant-design-vue';
import * as echarts from 'echarts';

import {
  buildInstrumentSelectOptionLabel,
  messagePrompt,
  useDashboardBodySize,
} from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/uiUtils';
import KfDashboardItem from '@kungfu-trader/kungfu-app/src/renderer/components/public/KfDashboardItem.vue';
import { KF_CONFIG_DIR } from '@kungfu-trader/kungfu-js-api/config/pathConfig';
import {
  dealKfTime,
  hashInstrumentUKey,
} from '@kungfu-trader/kungfu-js-api/kungfu';
import VueI18n from '@kungfu-trader/kungfu-js-api/language';
import {
  ExchangeIds,
  sideOffsetMap,
} from '@kungfu-trader/kungfu-js-api/config/tradingConfig';
import {
  OffsetEnum,
  SideEnum,
} from '@kungfu-trader/kungfu-js-api/typings/enums';
import {
  debounce,
  delayMilliSeconds,
} from '@kungfu-trader/kungfu-js-api/utils/busiUtils';

const { t } = VueI18n.global;

type WithTableRowInfo<T> = T & {
  tableRowId: string;
  msgTypeName: string;
  index?: number;
};
type QuoteChartResolved = WithTableRowInfo<KungfuApi.Quote>;
type OrderInputChartResolved = WithTableRowInfo<KungfuApi.OrderInput>;
type OrderChartResolved = WithTableRowInfo<KungfuApi.Order>;
type OrderActionResolved = WithTableRowInfo<KungfuApi.OrderAction> & {
  instrument_id: string;
  exchange_id: string;
  limit_price: number;
};

type FrameDataType =
  | KungfuApi.Quote
  | KungfuApi.OrderInput
  | KungfuApi.Order
  | KungfuApi.OrderAction;
type FrameResolvedDataType =
  | QuoteChartResolved
  | OrderInputChartResolved
  | OrderChartResolved
  | OrderActionResolved;

const props = withDefaults(
  defineProps<{
    category: string;
  }>(),
  {
    category: 'strategy',
  },
);

const DEFAULT_ORDER_LENGTH = 30;
const DEFAULT_CHART_LENGTH_RATE = 20;

const { setCurrentSession, setSelectedChartItem } = useJournalStore();
const {
  sessions,
  isLoadingFrames,
  currentSessionKey,
  currentFrameList,
  currentFrame,
  currentTime,
} = storeToRefs(useJournalStore());
const { dashboardBodyHeight } = useDashboardBodySize();
const columns = getStrategyColumns();
const simpleImage = Empty.PRESENTED_IMAGE_SIMPLE;
const kfInstrumentsJSON: Record<string, KungfuApi.InstrumentResolved> =
  fse.readJsonSync(path.join(KF_CONFIG_DIR, 'defaultInstruments.json'));
const searchInstrument = ref<string>('');
const selectedInstrument = ref<string>('');
const xAxisData = ref<Record<string, number[]>>({});
const quoteXAxisData = ref<Record<string, number[]>>({});
const searchOrderId = ref<string>('');
const instrumentList = ref<string[]>([]);

onMounted(() => {
  init();
});

onBeforeUnmount(() => {
  window.removeEventListener('resize', handleResize);
  myChart && myChart.dispose();
});

const strategyData = computed(() => {
  return sessions.value.filter((item) => {
    return item.category === props.category;
  });
});

const customRow = (record: KungfuApi.SessionResolved) => {
  return {
    onClick: () => {
      setCurrentSession(record);
      init();
    },
  };
};

const dealRowClassName = (row) => {
  return row.begin_time === currentSessionKey.value
    ? 'current-global-kfLocation'
    : '';
};

watch(
  () => isLoadingFrames.value,
  (newValue) => {
    if (!newValue) {
      init();
    }
  },
);

watch(
  () => currentFrame.value,
  (newCurrentFram) => {
    if (newCurrentFram) {
      let dataTime = 0n;
      let hasOrderId = false;
      if (
        ['Quote', 'OrderInput', 'Order', 'OrderAction'].includes(
          newCurrentFram.msgTypeName,
        )
      ) {
        const chartData = newCurrentFram.data as FrameResolvedDataType;
        let orderId: string | bigint = 0n;
        switch (newCurrentFram.msgTypeName) {
          case 'Quote':
            if ('data_time' in chartData) dataTime = chartData.data_time;
            orderId = newCurrentFram.id;
            break;
          case 'OrderInput':
            if ('insert_time' in chartData) dataTime = chartData.insert_time;
            if ('order_id' in chartData) orderId = chartData.order_id;
            break;
          case 'Order':
            if ('insert_time' in chartData) dataTime = chartData.insert_time;
            if ('order_id' in chartData) orderId = chartData.order_id;
            break;
          case 'OrderAction':
            if ('insert_time' in chartData) dataTime = chartData.insert_time;
            if ('order_id' in chartData) orderId = chartData.order_id;
            break;
        }

        if (newCurrentFram.msgTypeName === 'Quote') {
          const closestTimeIndex = findClosestTime(
            Number(dataTime),
            quoteXAxisData.value[selectedInstrument.value],
          );
          delayMilliSeconds(500)
            .then(() => {
              myChart.dispatchAction({
                type: 'highlight',
                seriesIndex: 0,
                dataIndex: closestTimeIndex,
              });
              return delayMilliSeconds(3000);
            })
            .then(() => {
              myChart.dispatchAction({
                type: 'downplay',
                seriesIndex: 0,
                dataIndex: closestTimeIndex,
              });
            })
            .catch((error) => {
              messagePrompt().error(error);
            });
        } else {
          selectedOrderId = orderId as bigint;
          option.series
            .filter((serie, index) => {
              return index !== 0;
            })
            .forEach((serie) => {
              serie.data.forEach((item) => {
                if (item.customInfo.orderId === orderId) {
                  hasOrderId = true;
                  item.symbolSize = 20;
                  let shadowColor = '';
                  if (item.customInfo.msgTypeName === 'orderAction') {
                    shadowColor = '#73F3F6';
                  } else {
                    shadowColor =
                      item.itemStyle?.color === '#f21717'
                        ? '#f37370'
                        : '#8fd460';
                  }
                  item.itemStyle = {
                    ...item.itemStyle,
                    shadowBlur: 10,
                    shadowColor,
                  };
                } else {
                  item.symbolSize = 10;
                  item.itemStyle = {
                    ...item.itemStyle,
                    shadowBlur: 0,
                  };
                }
              });
            });

          if (!hasOrderId) {
            messagePrompt().error(t('journalConfig.search_order_id_error'));
            return;
          }
        }
      } else {
        dataTime = newCurrentFram.genTime;
      }

      setDataZoom(dataTime);
      myChart && myChart.setOption(option);
    }
  },
);

function reset() {
  frameListResolved.value = {};
  chartSeriesData.value = {};
  xAxisData.value = {};
  quoteXAxisData.value = {};
  orderInfoMap.value = {};
  searchInstrument.value = '';
  searchOrderId.value = '';
  selectedInstrument.value = '';
}

function init() {
  reset();

  currentFrameList.value.forEach((item) => {
    if (item.msgTypeName === 'Order') {
      const tradingData = item.data as KungfuApi.Order;
      orderInfoMap.value[tradingData.order_id.toString()] = {
        instrumentId: tradingData.instrument_id,
        exchangeId: tradingData.exchange_id,
        limitPrice: tradingData.limit_price,
      };
    }
  });

  dealFrameData();
  instrumentList.value = getInstrumentList();

  if (instrumentList.value.length > 0) {
    getCurInstrument(instrumentList.value[0]);
    if (!myChart) {
      nextTick(() => {
        initChart();
      });
    }
  }
}

const orderInfoMap = ref<
  Record<
    string,
    {
      instrumentId: string;
      exchangeId: string;
      limitPrice: number;
    }
  >
>({});

const frameListResolved = ref<
  Record<
    string,
    {
      Quote: QuoteChartResolved[];
      OrderInput: OrderInputChartResolved[];
      Order: OrderChartResolved[];
      OrderAction: OrderActionResolved[];
    }
  >
>({});

const chartSeriesData = ref<
  Record<
    string,
    {
      Quote: SeriesData[];
      OrderInput: SeriesData[];
      Order: SeriesData[];
      OrderAction: SeriesData[];
    }
  >
>({});

function dealFrameData() {
  currentFrameList.value.forEach((item, index) => {
    if (
      !['Quote', 'OrderInput', 'Order', 'OrderAction'].includes(
        item.msgTypeName,
      )
    )
      return;

    let tradingData = item.data as FrameDataType;
    let tradingDataResolved: FrameResolvedDataType | null = null;
    const { dataTime } = getTradingDataValueByKey(
      tradingData,
      item.msgTypeName,
    );

    if (dataTime < currentTime.value) return;

    let uidKey = '';
    let key = '';
    if (item.msgTypeName === 'OrderAction') {
      tradingData = tradingData as KungfuApi.OrderAction;
      const { instrumentId, exchangeId, limitPrice } =
        orderInfoMap.value[tradingData.order_id.toString()] ?? {};
      if (!instrumentId || !exchangeId) {
        return;
      }
      uidKey = hashInstrumentUKey(instrumentId, exchangeId);

      tradingDataResolved = {
        ...tradingData,
        instrument_id: instrumentId,
        exchange_id: exchangeId,
        limit_price: limitPrice,
        tableRowId: item.id,
        msgTypeName: item.msgTypeName,
        index,
      };

      key = kfInstrumentsJSON[uidKey]
        ? buildInstrumentSelectOptionLabel(kfInstrumentsJSON[uidKey])
        : `${instrumentId} ${
            ExchangeIds[exchangeId.toUpperCase()]?.name || ''
          }`;
    } else {
      if ('instrument_id' in tradingData) {
        uidKey = hashInstrumentUKey(
          tradingData.instrument_id,
          tradingData.exchange_id,
        );

        tradingDataResolved = {
          ...tradingData,
          tableRowId: item.id,
          msgTypeName: item.msgTypeName,
          index,
        };

        key = kfInstrumentsJSON[uidKey]
          ? buildInstrumentSelectOptionLabel(kfInstrumentsJSON[uidKey])
          : `${tradingData.instrument_id} ${
              ExchangeIds[tradingData.exchange_id.toUpperCase()]?.name || ''
            }`;
      }
    }
    if (!key) {
      return;
    }
    if (!frameListResolved.value[key]) {
      frameListResolved.value[key] = {
        Quote: [],
        Order: [],
        OrderInput: [],
        OrderAction: [],
      };
    }
    if (!chartSeriesData.value[key]) {
      chartSeriesData.value[key] = {
        Quote: [],
        Order: [],
        OrderInput: [],
        OrderAction: [],
      };
    }
    frameListResolved.value[key][item.msgTypeName].push(tradingDataResolved);
    if (tradingDataResolved) {
      const { dataTime, price } = getTradingDataValueByKey(tradingDataResolved);

      if (!xAxisData.value[key]) {
        xAxisData.value[key] = [];
      } else {
        xAxisData.value[key].push(Number(dataTime));
      }

      if (item.msgTypeName === 'Quote') {
        if (!quoteXAxisData.value[key]) {
          quoteXAxisData.value[key] = [];
        }
        quoteXAxisData.value[key].push(Number(dataTime));
        tradingDataResolved = tradingDataResolved as QuoteChartResolved;
        chartSeriesData.value[key].Quote.push({
          value: [dealKfTime(dataTime), price],
          tooltip: {
            position: 'bottom',
            formatter: tooltipFormatter(tradingDataResolved, 'Quote'),
          },
          customInfo: {
            tableRowId: tradingDataResolved.tableRowId,
            time: tradingDataResolved.data_time,
            msgTypeName: tradingDataResolved.msgTypeName,
          },
        });
      } else if (item.msgTypeName === 'OrderInput') {
        tradingDataResolved = tradingDataResolved as OrderInputChartResolved;
        chartSeriesData.value[key].OrderInput.push({
          value: [dealKfTime(dataTime), price],
          symbolRotate: tradingDataResolved.side === 0 ? 180 : 0,
          itemStyle: {
            color: tradingDataResolved.side === 0 ? '#f21717' : '#17b07f',
          },
          tooltip: {
            position: 'bottom',
            formatter: tooltipFormatter(tradingDataResolved),
          },
          customInfo: {
            orderId: tradingDataResolved.order_id || 0n,
            tableRowId: tradingDataResolved.tableRowId,
            time: tradingDataResolved.insert_time,
            msgTypeName: tradingDataResolved.msgTypeName,
          },
        });
      } else if (item.msgTypeName === 'Order') {
        tradingDataResolved = tradingDataResolved as OrderChartResolved;
        chartSeriesData.value[key].Order.push({
          value: [dealKfTime(dataTime), price],
          symbolRotate: tradingDataResolved.side === 0 ? 180 : 0,
          symbolOffset:
            tradingDataResolved.side === 0 ? [0, '-160%'] : [0, '160%'],
          itemStyle: {
            color: tradingDataResolved.side === 0 ? '#f21717' : '#17b07f',
          },
          tooltip: {
            position: 'bottom',
            formatter: tooltipFormatter(tradingDataResolved),
          },
          customInfo: {
            orderId: tradingDataResolved.order_id || 0n,
            tableRowId: tradingDataResolved.tableRowId,
            time: tradingDataResolved.insert_time,
            msgTypeName: tradingDataResolved.msgTypeName,
          },
          label: {
            show: true,
            position: tradingDataResolved.side === 0 ? 'top' : 'bottom',
            color: tradingDataResolved.side === 0 ? '#f21717' : '#17b07f',
            formatter: () => {
              const side =
                (tradingDataResolved as OrderChartResolved).side ??
                SideEnum.Unknown;
              const offset =
                (tradingDataResolved as OrderChartResolved).offset ??
                OffsetEnum.Unknown;

              return sideOffsetMap[side]
                ? sideOffsetMap[side][offset] || '--'
                : '--';
            },
          },
        });
      } else if (item.msgTypeName === 'OrderAction') {
        tradingDataResolved = tradingDataResolved as OrderActionResolved;
        chartSeriesData.value[key].OrderAction.push({
          value: [dealKfTime(dataTime), price],
          tooltip: {
            position: 'bottom',
            formatter: tooltipFormatter(tradingDataResolved),
          },
          customInfo: {
            orderId: tradingDataResolved.order_id || 0n,
            tableRowId: tradingDataResolved.tableRowId,
            time: tradingDataResolved.insert_time,
            msgTypeName: tradingDataResolved.msgTypeName,
          },
        });
      }
    }
  });
}

function getInstrumentList(searchKey?: string) {
  return searchKey
    ? Object.keys(frameListResolved.value).filter((item) => {
        return item.includes(searchKey);
      })
    : Object.keys(frameListResolved.value);
}

function getCurInstrument(instrument: string) {
  selectedInstrument.value = instrument;
  selectedOrderId = 0n;
  searchOrderId.value = '';

  updateOption();
}

const chartWrapper = ref<HTMLElement>();
let myChart: echarts.ECharts;
let selectedOrderId = 0n;
const option = getChartOption();
const xAxisMinMax = ref<{
  min: number | string;
  max: number | string;
}>({
  min: 'dataMin',
  max: 'dataMax',
});

const initChart = () => {
  const element = document.getElementById('strategyChart');
  if (element) {
    myChart = echarts.init(element as HTMLElement);

    addChartEventListener(myChart);
    myChart.setOption(option);
  }
};

const updateOption = () => {
  setXAxisMinMax();

  option.yAxis.min = xAxisMinMax.value.min;
  option.yAxis.max = xAxisMinMax.value.max;

  option.series[0].data = chartSeriesData.value[selectedInstrument.value].Quote;
  option.series[1].data =
    chartSeriesData.value[selectedInstrument.value].OrderInput;
  option.series[2].data = chartSeriesData.value[selectedInstrument.value].Order;
  option.series[3].data =
    chartSeriesData.value[selectedInstrument.value].OrderAction;

  const dataLength = option.series[1].data.length;
  if (dataLength <= DEFAULT_ORDER_LENGTH) {
    option.dataZoom.forEach((item) => {
      item.start = 0;
      item.end = 100;
    });
  } else {
    option.dataZoom.forEach((item) => {
      item.start = 0;
      item.end =
        (DEFAULT_ORDER_LENGTH / dataLength) * 100 < DEFAULT_CHART_LENGTH_RATE
          ? DEFAULT_CHART_LENGTH_RATE
          : (DEFAULT_ORDER_LENGTH / dataLength) * 100;
    });
  }

  option.xAxis.data = xAxisData.value[selectedInstrument.value]
    .sort((a, b) => {
      return a - b;
    })
    .map((item) => {
      return dealKfTime(BigInt(item));
    });
  quoteXAxisData.value[selectedInstrument.value]?.sort((a, b) => {
    return a - b;
  });

  myChart && myChart.setOption(option);
};

function handleResize() {
  myChart && myChart.resize();
}

function addChartEventListener(myChart: echarts.ECharts) {
  window.addEventListener('resize', handleResize);

  myChart.on('click', (params) => {
    if (!option || !params.data) return;
    const serieItemData = params.data as SeriesData;
    const { msgTypeName, tableRowId, orderId } = serieItemData.customInfo;

    frameListResolved.value[selectedInstrument.value][msgTypeName].forEach(
      (fram) => {
        if (fram.tableRowId === tableRowId) {
          setSelectedChartItem(fram.index ?? 0);
        }
      },
    );

    if (params.componentSubType === 'scatter') {
      selectedOrderId = orderId || 0n;
      option.series
        .filter((serie, index) => {
          return index !== 0;
        })
        .forEach((serie) => {
          serie.data.forEach((item: SeriesData) => {
            if (item.customInfo.orderId === orderId) {
              let shadowColor = '';
              item.symbolSize = 20;
              if (item.customInfo.msgTypeName === 'orderAction') {
                shadowColor = '#73F3F6';
              } else {
                shadowColor =
                  item.itemStyle?.color === '#f21717' ? '#f37370' : '#8fd460';
              }
              item.itemStyle = {
                ...item.itemStyle,
                shadowBlur: 10,
                shadowColor,
              };
            } else {
              item.symbolSize = 10;
              item.itemStyle = {
                ...item.itemStyle,
                shadowBlur: 0,
              };
            }
          });
        });

      myChart && myChart.setOption(option);
    }
  });

  myChart.on('datazoom', (params) => {
    let { start, end, batch } = params as {
      start: number;
      end: number;
      batch: { start: number; end: number }[];
    };
    if (!start || !end) {
      start = batch ? (batch[0] ? batch[0].start : 0) : 0;
      end = batch ? (batch[0] ? batch[0].end : 0) : 100;
    }

    option.dataZoom.forEach((item) => {
      item.start = start;
      item.end = end;
    });
  });

  myChart.getZr().on('click', (event) => {
    if (!event.target) {
      if (!Number(selectedOrderId)) return;
      option.series
        .filter((serie, index) => {
          return index !== 0;
        })
        .forEach((serie) => {
          serie.data.forEach((item) => {
            item.symbolSize = 10;
            item.itemStyle = {
              ...item.itemStyle,
              shadowBlur: 0,
            };
          });
        });
      myChart && myChart.setOption(option);
      selectedOrderId = 0n;
    }
  });

  let lastIndex;
  myChart.getZr().on('mousemove', (e) => {
    if (!e.target || e.target.type !== 'ec-polyline') return;
    const { offsetX, offsetY } = e;
    const [index] = myChart.convertFromPixel({ seriesIndex: 0 }, [
      offsetX,
      offsetY,
    ]);

    let { dataTime } = getTradingDataValueByKey(chartFrameList.value[index]);
    let closestTimeIndex = findClosestTime(
      Number(dataTime),
      quoteXAxisData.value[selectedInstrument.value],
    );
    if (lastIndex !== undefined) {
      myChart.dispatchAction({
        type: 'downplay',
        seriesIndex: 0,
        dataIndex: lastIndex,
      });
    }
    myChart.dispatchAction({
      type: 'highlight',
      seriesIndex: 0,
      dataIndex: closestTimeIndex,
    });
    if (closestTimeIndex !== lastIndex) lastIndex = closestTimeIndex;
  });

  const element = document.getElementById('strategyChart');
  element &&
    element.addEventListener('mouseout', () => {
      if (lastIndex) {
        myChart.dispatchAction({
          type: 'downplay',
          seriesIndex: 0,
          dataIndex: lastIndex,
        });
      }
    });
}

const chartFrameList = computed(() => {
  return [
    ...(frameListResolved.value[selectedInstrument.value].Quote ?? []),
    ...(frameListResolved.value[selectedInstrument.value].Order ?? []),
    ...(frameListResolved.value[selectedInstrument.value].OrderInput ?? []),
    ...(frameListResolved.value[selectedInstrument.value].OrderAction ?? []),
  ].sort((a, b) => {
    let aDataTime = getTradingDataValueByKey(a).dataTime;
    let bDataTime = getTradingDataValueByKey(b).dataTime;
    return Number(aDataTime) - Number(bDataTime);
  });
});

function getTradingDataValueByKey(
  data: FrameResolvedDataType | FrameDataType,
  type?: string,
) {
  let dataTime = 0n,
    price = 0;
  if (!data)
    return {
      dataTime,
      price,
    };
  let msgTypeName = '';
  if ('msgTypeName' in data) {
    msgTypeName = data.msgTypeName;
  } else if (type) {
    msgTypeName = type;
  } else {
    return {
      dataTime,
      price,
    };
  }
  switch (msgTypeName) {
    case 'Quote':
      if ('data_time' in data) dataTime = data.data_time;
      if ('last_price' in data) price = data.last_price;
      break;
    case 'OrderInput':
      if ('insert_time' in data) dataTime = data.insert_time;
      if ('limit_price' in data) price = data.limit_price;
      break;
    case 'Order':
      if ('insert_time' in data) dataTime = data.insert_time;
      if ('limit_price' in data) price = data.limit_price;
      break;
    case 'OrderAction':
      if ('insert_time' in data) dataTime = data.insert_time;
      if ('limit_price' in data) price = data.limit_price;
      break;
  }

  return {
    dataTime,
    price,
  };
}

function tooltipFormatter(data: FrameResolvedDataType, type?: string) {
  let htmlTemplate;
  if (type) {
    htmlTemplate = Object.keys(data).reduce((pre, cur) => {
      if (cur === 'bid_volume' || cur === 'ask_volume') return pre;
      if (cur === 'bid_price' || cur === 'ask_price') {
        const side =
          cur === 'bid_price'
            ? t('tradingConfig.buy')
            : t('tradingConfig.sell');
        data[cur].forEach((item, index) => {
          if (index > 4) return;
          const volume =
            cur === 'bid_price'
              ? data['bid_volume'][index]
              : data['ask_volume'][index];
          pre += `<div class="tooltip-row">
          <span class="tooltip-item-key">${side}${index + 1}</span>
          <span class="tooltip-item-value">${item}--${volume}</span>
        </div>`;
        });

        return pre;
      }
      return (pre += `<div class="tooltip-row">
          <span class="tooltip-item-key">${cur}</span>
          <span class="tooltip-item-value">${
            cur === 'data_time' ? dealKfTime(data[cur]) : data[cur]
          }</span>
        </div>`);
    }, '');
  } else {
    htmlTemplate = Object.keys(data).reduce((pre, cur) => {
      return (pre += `<div class="tooltip-row">
          <span class="tooltip-item-key">${cur}</span>
          <span class="tooltip-item-value">${
            ['insert_time', 'update_time'].includes(cur)
              ? dealKfTime(data[cur])
              : data[cur]
          }</span>
        </div>`);
    }, '');
  }
  return `
    <div class="tooltip-container">
      ${htmlTemplate}
    </div>
  `;
}

function findClosestTime(targetTime: number, times: number[]) {
  if (times.length === 0) {
    return 0;
  }

  let closestIndex = 0;
  let closestDiff = Math.abs(targetTime - times[0]);

  times.forEach((item, index) => {
    const currentDiff = Math.abs(targetTime - item);
    if (currentDiff < closestDiff) {
      closestDiff = currentDiff;
      closestIndex = index;
    }
  });

  return closestIndex;
}

function handleSearchOrderId() {
  if (!searchOrderId.value) return;
  const orderIdInfo = {
    hasId: false,
    tableRowId: '',
    msgTypeName: '',
  };
  let dataTime = 0n;

  option.series
    .filter((serie, index) => {
      return index !== 0;
    })
    .forEach((serie) => {
      serie.data.forEach((item) => {
        if (item.customInfo.orderId === BigInt(searchOrderId.value)) {
          orderIdInfo.hasId = true;
          orderIdInfo.msgTypeName = item.customInfo.msgTypeName;
          orderIdInfo.tableRowId = item.customInfo.tableRowId;

          dataTime = item.customInfo.time;
          let shadowColor = '';
          item.symbolSize = 20;
          if (item.customInfo.msgTypeName === 'orderAction') {
            shadowColor = '#73F3F6';
          } else {
            shadowColor =
              item.itemStyle?.color === '#f21717' ? '#f37370' : '#8fd460';
          }
          item.itemStyle = {
            ...item.itemStyle,
            shadowBlur: 10,
            shadowColor,
          };
        } else {
          item.symbolSize = 10;
          item.itemStyle = {
            ...item.itemStyle,
            shadowBlur: 0,
          };
        }
      });
    });
  if (!orderIdInfo.hasId) {
    messagePrompt().error(t('journalConfig.search_order_id_error'));
    return;
  }

  frameListResolved.value[selectedInstrument.value][
    orderIdInfo.msgTypeName
  ].forEach((fram) => {
    if (fram.id === orderIdInfo.tableRowId) {
      setSelectedChartItem(fram.index ?? 0);
    }
  });

  selectedOrderId = BigInt(searchOrderId.value);
  setDataZoom(dataTime);
  myChart && myChart.setOption(option);
}

function setDataZoom(dataTime: bigint) {
  const closestTimeIndex = findClosestTime(
    Number(dataTime),
    xAxisData.value[selectedInstrument.value],
  );
  const start = (
    (closestTimeIndex / xAxisData.value[selectedInstrument.value].length) *
    100
  ).kfRound(2);
  if (start <= 15) {
    option.dataZoom.forEach((item) => {
      item.start = 0;
      item.end = 20;
    });
  } else {
    option.dataZoom.forEach((item) => {
      item.start = start - 10;
      item.end = start + 10;
    });
  }
}

function setXAxisMinMax() {
  if (frameListResolved.value[selectedInstrument.value].Quote.length > 0) {
    const { upper_limit_price, lower_limit_price, last_price } =
      frameListResolved.value[selectedInstrument.value].Quote[0];
    xAxisMinMax.value.max = upper_limit_price
      ? Math.floor(upper_limit_price)
      : last_price
      ? Math.floor(last_price * 1.2)
      : 'dataMax';
    xAxisMinMax.value.min = lower_limit_price
      ? Math.floor(lower_limit_price)
      : last_price
      ? Math.floor(last_price * 0.8)
      : 'dataMin';
  } else {
    let limit_price;
    if (
      frameListResolved.value[selectedInstrument.value].OrderInput.length > 0
    ) {
      limit_price =
        frameListResolved.value[selectedInstrument.value].OrderInput[0]
          .limit_price;
    } else if (
      frameListResolved.value[selectedInstrument.value].Order.length > 0
    ) {
      limit_price =
        frameListResolved.value[selectedInstrument.value].Order[0].limit_price;
    } else {
      xAxisMinMax.value = {
        max: 'dataMax',
        min: 'dataMin',
      };
      return;
    }

    xAxisMinMax.value = {
      max: Math.floor(limit_price * 1.2),
      min: Math.floor(limit_price * 0.8),
    };
  }
}

const handleInputChange = debounce(() => {
  instrumentList.value = getInstrumentList(searchInstrument.value);
}, 300);
</script>

<style lang="less">
.kf-visualization_wrap {
  position: relative;
  display: flex;
  .ant-spin.ant-spin-spinning {
    position: absolute;
  }

  .kf-journal-spin {
    .ant-spin-text {
      margin-left: 8px;
    }
  }
  .ant-empty-normal {
    padding: 32px 0;
    margin: 0;
  }
  .ant-table {
    background-color: #1d1d1d;
    .ant-table-cell-fix-left,
    .ant-table-cell-fix-right {
      background-color: #1d1d1d;
    }
  }
  .kf-strategy_wrap {
    flex: 0 0 400px;
  }
  .kf-instrument_wrap {
    flex: 0 0 200px;
    margin: 0 4px;
    background-color: #1d1d1d;
    .search-input {
      width: 100%;
      min-height: 28px;
      line-height: 28px;
    }
    .instrument-list {
      overflow: auto;
      height: calc(100% - 28px);
      .instrument-item_wrap {
        line-height: 28px;
        height: 28px;
        padding: 0 4px;
        font-size: 12px;
        text-align: left;
        cursor: pointer;
        &:hover {
          background: #434343;
        }
      }
      .selected-status {
        background: #434343;
      }
      .instrument-item {
        margin-right: 2px;
      }
    }
  }
  .kf-chart_wrap {
    flex: 1;
    overflow: visible;
    position: relative;
    background-color: #1d1d1d;
    .chart-search-order-id {
      position: absolute;
      top: 0;
      right: 0;
      width: 20%;
      max-width: 300px;
      z-index: 999;
    }
    .kf-chart_content {
      width: 100%;
      height: 100%;
    }
    .tooltip-container {
      width: 400px;
      color: #ffffffd9;
      .tooltip-row {
        display: flex;
        justify-content: space-between;
      }
    }
  }
}
</style>

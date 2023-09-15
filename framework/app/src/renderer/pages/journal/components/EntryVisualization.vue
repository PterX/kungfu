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
        v-model:value="searchOrderId"
        class="chart-search-order-id"
        :placeholder="$t('journalConfig.search_order_id')"
        @search="handleSearchOrderId"
      />
      <div id="strategyChart" class="kf-chart_content"></div>
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

type WithTableRowInfo<T> = T & { tableRowId: string; msgTypeName: string };
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

const { setCurrentSession, setSelectedChartItem } = useJournalStore();
const {
  sessions,
  isLoadingFrames,
  currentSessionKey,
  currentFrameList,
  currentFrame,
} = storeToRefs(useJournalStore());
const { dashboardBodyHeight } = useDashboardBodySize();
const columns = getStrategyColumns();
const simpleImage = Empty.PRESENTED_IMAGE_SIMPLE;
const kfInstrumentsJSON: Record<string, KungfuApi.InstrumentResolved> =
  fse.readJsonSync(path.join(KF_CONFIG_DIR, 'defaultInstruments.json'));
const searchInstrument = ref<string>('');

const quoteByInstrument = ref<Record<string, QuoteChartResolved[]>>({});
const orderInputByInstrument = ref<Record<string, OrderInputChartResolved[]>>(
  {},
);
const orderByInstrument = ref<Record<string, OrderChartResolved[]>>({});
const orderActionByInstrument = ref<Record<string, OrderActionResolved[]>>({});

const selectedInstrument = ref<string>('');
const xAxisData = ref<number[]>([]);
const quoteXAxisData = ref<number[]>([]);
const searchOrderId = ref<string>('');
const instrumentList = ref<string[]>([]);

onMounted(() => {
  dealAllFrameData();
  nextTick(() => {
    initChart();
  });
});

onBeforeUnmount(() => {
  window.removeEventListener('resize', handleResize);
  myChart && myChart.dispose();
});

const strategyData = computed(() => {
  return sessions.value.filter((item) => {
    return item.category === 'strategy';
  });
});

const customRow = (record: KungfuApi.SessionResolved) => {
  return {
    onClick: () => {
      setCurrentSession(record);
      dealAllFrameData();
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
      dealAllFrameData();
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
            if ('update_time' in chartData) dataTime = chartData.update_time;
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
            quoteXAxisData.value,
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
                  if (item.itemStyle) {
                    item.itemStyle = {
                      ...item.itemStyle,
                      shadowBlur: 30,
                      shadowColor: item.itemStyle.color,
                    };
                  }
                } else {
                  item.symbolSize = 10;
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
  orderByInstrument.value = {};
  quoteByInstrument.value = {};
  orderInputByInstrument.value = {};
  searchInstrument.value = '';
  searchOrderId.value = '';
  selectedInstrument.value = '';
  xAxisData.value = [];
}

function dealAllFrameData() {
  reset();
  dealFrameListMap();
  quoteByInstrument.value = dealFrame<QuoteChartResolved>(
    'quote',
    kfInstrumentsJSON,
  );
  orderByInstrument.value = dealFrame<OrderChartResolved>(
    'order',
    kfInstrumentsJSON,
  );
  orderInputByInstrument.value = dealFrame<OrderInputChartResolved>(
    'orderInput',
    kfInstrumentsJSON,
  );
  orderActionByInstrument.value = dealFrame<OrderActionResolved>(
    'orderAction',
    kfInstrumentsJSON,
  );

  instrumentList.value = getInstrumentList();

  if (instrumentList.value.length > 0) {
    getCurInstrument(instrumentList.value[0]);
  }
}

const chartFrameListMap = ref<Record<string, KungfuApi.FrameResolved[]>>({
  quote: [],
  orderInput: [],
  order: [],
  orderAction: [],
});

function dealFrameListMap() {
  chartFrameListMap.value = {
    quote: [],
    orderInput: [],
    order: [],
    orderAction: [],
  };

  let requestStart = false;
  currentFrameList.value.forEach((item, index) => {
    if (item.msgTypeName === 'RequestStart') {
      requestStart = true;
    }
    if (requestStart) {
      item.index = index;
      if (item.msgTypeName === 'Quote') {
        chartFrameListMap.value.quote.push(item);
      } else if (item.msgTypeName === 'OrderInput') {
        chartFrameListMap.value.orderInput.push(item);
      } else if (item.msgTypeName === 'Order') {
        chartFrameListMap.value.order.push(item);
      } else if (item.msgTypeName === 'OrderAction') {
        chartFrameListMap.value.orderAction.push(item);
      }
    }
  });
}

function dealFrame<T extends FrameResolvedDataType>(
  msgTypeName: string,
  kfInstrumentsJSON: Record<string, KungfuApi.InstrumentResolved>,
): Record<string, T[]> {
  const hasInstrumentId: string[] = [];
  const result: Record<string, T[]> = {};
  const frameList = chartFrameListMap.value[msgTypeName];
  let orders: KungfuApi.Order[] = [];

  if (msgTypeName === 'orderAction') {
    orders = chartFrameListMap.value['order'].map((frame) => {
      return frame.data as KungfuApi.Order;
    });
  }

  frameList.forEach((item) => {
    let tradingData = {} as T;
    if (msgTypeName === 'orderAction') {
      const orderAction = item.data as KungfuApi.OrderAction;
      const order = orders.filter((order) => {
        return orderAction.order_id === order.order_id;
      })[0];

      if (!order) {
        messagePrompt().error('数据异常');
      }
      tradingData = {
        ...orderAction,
        instrument_id: order.instrument_id,
        exchange_id: order.exchange_id,
        limit_price: order.limit_price,
        tableRowId: item.id,
        msgTypeName,
      } as T;
    } else {
      tradingData = {
        ...(item.data as FrameDataType),
        tableRowId: item.id,
        msgTypeName,
      } as T;
    }

    const uidKey = hashInstrumentUKey(
      tradingData.instrument_id,
      tradingData.exchange_id,
    );
    const key = kfInstrumentsJSON[uidKey]
      ? buildInstrumentSelectOptionLabel(kfInstrumentsJSON[uidKey])
      : `${tradingData.instrument_id} ${
          ExchangeIds[tradingData.exchange_id.toUpperCase()]?.name || ''
        }`;

    if (hasInstrumentId.includes(tradingData.instrument_id)) {
      result[key].push(tradingData);
    } else {
      hasInstrumentId.push(tradingData.instrument_id);
      result[key] = [tradingData];
    }
  });

  return result;
}

function dealChartDataByFrameResolved<T extends FrameResolvedDataType>(
  selectedInstrument: string,
  frameResolved: Record<string, FrameResolvedDataType[]>,
  type: string,
): {
  value: (string | number)[];
  data: T;
}[] {
  if (Object.keys(frameResolved).length === 0 || !selectedInstrument) return [];

  const tradingData = frameResolved[selectedInstrument];
  if (!tradingData) return [];

  const timeData = tradingData.map((item) => {
    let dataTime = 0n;
    switch (type) {
      case 'Quote':
        if ('data_time' in item) dataTime = item.data_time;
        break;
      case 'OrderInput':
        if ('insert_time' in item) dataTime = item.insert_time;
        break;
      case 'Order':
        if ('update_time' in item) dataTime = item.update_time;
        break;
      case 'OrderAction':
        if ('insert_time' in item) dataTime = item.insert_time;
        break;
    }
    return Number(dataTime);
  });
  if (type === 'Quote') {
    quoteXAxisData.value = timeData;
  }
  xAxisData.value = [...xAxisData.value, ...timeData];

  return frameResolved[selectedInstrument].map((item) => {
    let time = 0n,
      price = 0;
    switch (type) {
      case 'Quote':
        if ('data_time' in item) time = item.data_time;
        if ('last_price' in item) price = item.last_price;
        break;
      case 'OrderInput':
        if ('insert_time' in item) time = item.insert_time;
        if ('limit_price' in item) price = item.limit_price;
        break;
      case 'Order':
        if ('update_time' in item) time = item.update_time;
        if ('limit_price' in item) price = item.limit_price;
        break;
      case 'OrderAction':
        if ('insert_time' in item) time = item.insert_time;
        if ('limit_price' in item) price = item.limit_price;
        break;
    }

    return {
      value: [dealKfTime(time), price],
      data: item as T,
    };
  });
}

function getInstrumentList(searchKey?: string) {
  const allInstrument = Array.from(
    new Set([
      ...Object.keys(orderByInstrument.value),
      ...Object.keys(quoteByInstrument.value),
      ...Object.keys(orderInputByInstrument.value),
    ]),
  );

  return searchKey
    ? allInstrument.filter((item) => {
        return item.includes(searchKey);
      })
    : allInstrument;
}

function getCurInstrument(instrument: string) {
  selectedInstrument.value = instrument;
  selectedOrderId = 0n;
  searchOrderId.value = '';
  xAxisData.value = [];

  setXAxisMinMax(instrument);

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
  option.yAxis.min = xAxisMinMax.value.min;
  option.yAxis.max = xAxisMinMax.value.max;

  option.dataZoom.forEach((item) => {
    item.start = 0;
    item.end = 20;
  });

  option.series[0].data = dealChartDataByFrameResolved<QuoteChartResolved>(
    selectedInstrument.value,
    quoteByInstrument.value,
    'Quote',
  ).map((item) => {
    return {
      value: item.value,
      tooltip: {
        position: 'bottom',
        formatter: tooltipFormatter(item.data, 'quote'),
      },
      customInfo: {
        tableRowId: item.data.tableRowId,
        time: item.data.data_time,
        msgTypeName: item.data.msgTypeName,
      },
    };
  });

  option.series[1].data = dealChartDataByFrameResolved<OrderInputChartResolved>(
    selectedInstrument.value,
    orderInputByInstrument.value,
    'OrderInput',
  ).map((item) => {
    return {
      value: item.value,
      symbolRotate: item.data.side === 0 ? 180 : 0,
      itemStyle: {
        color: item.data.side === 0 ? '#f21717' : '#17b07f',
      },
      tooltip: {
        position: 'bottom',
        formatter: tooltipFormatter(item.data),
      },
      customInfo: {
        orderId: item.data.order_id || 0n,
        tableRowId: item.data.tableRowId,
        time: item.data.insert_time,
        msgTypeName: item.data.msgTypeName,
      },
    };
  });

  option.series[2].data = dealChartDataByFrameResolved<OrderChartResolved>(
    selectedInstrument.value,
    orderByInstrument.value,
    'Order',
  ).map((item) => {
    return {
      value: item.value,
      symbolRotate: item.data.side === 0 ? 180 : 0,
      itemStyle: {
        color: item.data.side === 0 ? '#f21717' : '#17b07f',
      },
      tooltip: {
        position: 'bottom',
        formatter: tooltipFormatter(item.data),
      },
      symbolOffset: item.data.side === 0 ? [0, '-160%'] : [0, '160%'],
      customInfo: {
        orderId: item.data.order_id || 0n,
        tableRowId: item.data.tableRowId,
        time: item.data.update_time,
        msgTypeName: item.data.msgTypeName,
      },
      label: {
        show: true,
        position: item.data.side === 0 ? 'top' : 'bottom',
        color: item.data.side === 0 ? '#f21717' : '#17b07f',
        formatter: () => {
          const side = item.data.side ?? SideEnum.Unknown;
          const offset = item.data.offset ?? OffsetEnum.Unknown;

          return sideOffsetMap[side]
            ? sideOffsetMap[side][offset] || '--'
            : '--';
        },
      },
    };
  });

  option.series[3].data = dealChartDataByFrameResolved<OrderActionResolved>(
    selectedInstrument.value,
    orderActionByInstrument.value,
    'OrderAction',
  ).map((item) => {
    return {
      value: item.value,
      tooltip: {
        position: 'bottom',
        formatter: tooltipFormatter(item.data),
      },
      customInfo: {
        orderId: item.data.order_id || 0n,
        tableRowId: item.data.tableRowId,
        time: item.data.insert_time,
        msgTypeName: item.data.msgTypeName,
      },
    };
  });

  xAxisData.value.sort((a, b) => {
    return a - b;
  });

  const xAxis = xAxisData.value;
  option.xAxis.data = xAxis.map((item) => {
    return dealKfTime(BigInt(item));
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

    chartFrameListMap.value[msgTypeName].forEach((fram) => {
      if (fram.id === tableRowId) {
        setSelectedChartItem(fram.index ?? 0);
      }
    });

    if (params.componentSubType === 'scatter') {
      selectedOrderId = orderId || 0n;
      option.series
        .filter((serie, index) => {
          return index !== 0;
        })
        .forEach((serie) => {
          serie.data.forEach((item: SeriesData) => {
            if (item.customInfo.orderId === orderId) {
              item.symbolSize = 20;
              if (item.itemStyle) {
                item.itemStyle = {
                  ...item.itemStyle,
                  shadowBlur: 30,
                  shadowColor: item.itemStyle.color,
                };
              }
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
    const closestTimeIndex = findClosestTime(
      Number(dataTime),
      quoteXAxisData.value,
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
    ...(quoteByInstrument.value[selectedInstrument.value] ?? []),
    ...(orderByInstrument.value[selectedInstrument.value] ?? []),
    ...(orderInputByInstrument.value[selectedInstrument.value] ?? []),
    ...(orderActionByInstrument.value[selectedInstrument.value] ?? []),
  ].sort((a, b) => {
    let aDataTime = getTradingDataValueByKey(a).dataTime;
    let bDataTime = getTradingDataValueByKey(b).dataTime;
    return Number(aDataTime) - Number(bDataTime);
  });
});

function getTradingDataValueByKey(data: FrameResolvedDataType) {
  let dataTime = 0n;
  if (!data)
    return {
      dataTime,
    };
  switch (data.msgTypeName) {
    case 'quote':
      if ('data_time' in data) dataTime = data.data_time;
      break;
    case 'orderInput':
      if ('insert_time' in data) dataTime = data.insert_time;
      break;
    case 'order':
      if ('update_time' in data) dataTime = data.update_time;
      break;
    case 'orderAction':
      if ('insert_time' in data) dataTime = data.insert_time;
      break;
  }

  return {
    dataTime,
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
          item.symbolSize = 20;
          if (item.itemStyle) {
            item.itemStyle = {
              ...item.itemStyle,
              shadowBlur: 30,
              shadowColor: item.itemStyle.color,
            };
          }
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

  chartFrameListMap.value[orderIdInfo.msgTypeName].forEach((fram) => {
    if (fram.id === orderIdInfo.tableRowId) {
      setSelectedChartItem(fram.index ?? 0);
    }
  });

  selectedOrderId = BigInt(searchOrderId.value);
  setDataZoom(dataTime);
  myChart && myChart.setOption(option);
}

function setDataZoom(dataTime: bigint) {
  const closestTimeIndex = findClosestTime(Number(dataTime), xAxisData.value);
  const start = ((closestTimeIndex / xAxisData.value.length) * 100).kfRound(2);
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

function setXAxisMinMax(key: string) {
  if (quoteByInstrument.value[key]?.length > 0) {
    const { upper_limit_price, lower_limit_price, last_price } =
      quoteByInstrument.value[key][0];
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
    if (orderInputByInstrument.value[key]?.length > 0) {
      limit_price = orderInputByInstrument.value[key][0].limit_price;
    } else if (orderByInstrument.value[key]?.length > 0) {
      limit_price = orderByInstrument.value[key][0].limit_price;
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
      width: 300px;
      z-index: 999;
    }
    .kf-chart_content {
      width: 100%;
      height: 100%;
    }
    .tooltip-container {
      width: 450px;
      color: #ffffffd9;
      .tooltip-row {
        display: flex;
        justify-content: space-between;
      }
    }
  }
}
</style>

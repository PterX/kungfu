import { SessionStatusEnum } from '@kungfu-trader/kungfu-js-api/typings/enums';
import VueI18n from '@kungfu-trader/kungfu-js-api/language';

const { t } = VueI18n.global;

const circleSvg =
  'path://M12 2C6.5 2 2 6.5 2 12s4.5 10 10 10 10-4.5 10-10S17.5 2 12 2';
const arrowSvg =
  'path://M678.4 409.6l-147.2-192c-6.4-12.8-25.6-12.8-38.4 0l-147.2 192C339.2 428.8 345.6 448 364.8 448L448 448l0 352C448 819.2 460.8 832 480 832l64 0C563.2 832 576 819.2 576 800L576 448l83.2 0C678.4 448 684.8 428.8 678.4 409.6z';
const triangleSvg =
  'path://M529.493333 179.84l447.872 639.872a21.333333 21.333333 0 0 1-17.493333 33.578667H64.128a21.333333 21.333333 0 0 1-17.493333-33.578667L494.506667 179.84a21.333333 21.333333 0 0 1 34.986666 0z';
const forkSvg =
  'path://M617.92 516.096l272 272-101.824 101.824-272-272-272 272-101.856-101.824 272-272-275.008-275.04L241.056 139.2l275.04 275.04 275.04-275.04 101.824 101.824-275.04 275.04z';
const straightLine = 'path://M63.6 489.6h896.7v44.8H63.6z';

type TableDataType = KungfuApi.FrameResolved & KungfuApi.SessionResolved;

const buildSorter =
  (dataIndex: keyof TableDataType) => (a: TableDataType, b: TableDataType) =>
    +Number(a[dataIndex]) - +Number(b[dataIndex]);

const buildStrSorter =
  (dataIndex: keyof TableDataType) => (a: TableDataType, b: TableDataType) =>
    a[dataIndex].toString().localeCompare(b[dataIndex].toString());

export const getSessionColumns = (): AntTableColumn[] => [
  {
    title: t('journalConfig.session_id'),
    dataIndex: 'sessionName',
    align: 'left',
    width: 240,
    sorter: {
      compare: buildStrSorter('sessionName'),
    },
    fixed: 'left',
  },
  {
    title: t('journalConfig.begin_time'),
    dataIndex: 'beginTimeResolved',
    align: 'left',
    width: 160,
    sorter: {
      compare: buildSorter('begin_time'),
    },
    fixed: 'left',
  },
  {
    title: t('journalConfig.end_time'),
    dataIndex: 'endTimeResolved',
    width: 160,
    align: 'left',
    sorter: { compare: buildStrSorter('end_time') },
    fixed: 'left',
  },
  {
    title: t('journalConfig.status'),
    dataIndex: 'status',
    align: 'left',
    fixed: 'right',
  },
];

export const SessionStatus: Record<
  SessionStatusEnum,
  KungfuApi.KfTradeValueCommonData
> = {
  [SessionStatusEnum.Running]: {
    name: t('journalConfig.running'),
    color: '#FAAD14',
  },
  [SessionStatusEnum.Finished]: {
    name: t('journalConfig.finished'),
    color: 'gray',
  },
};

export const getFrameColumns = (
  searchInUsing: boolean,
): KfTradingDataTableHeaderConfig[] => [
  {
    type: 'string',
    name: t('journalConfig.gen_time'),
    dataIndex: 'genTimeResolved',
    width: 128,
    textOverflow: 'ellipsis',
    sorter: buildSorter('genTime'),
  },
  {
    type: 'string',
    name: `${t('journalConfig.source')} → ${t('journalConfig.dest')}`,
    dataIndex: 'sourceToDest',
    textOverflow: 'ellipsis',
    sorter: buildStrSorter('sourceToDest'),
    width: 216,
  },
  {
    type: 'string',
    name: t('journalConfig.initial_source'),
    dataIndex: 'initialSourceResolved',
    align: 'right',
    textOverflow: 'ellipsis',
    width: 180,
  },
  {
    type: 'string',
    name: t('journalConfig.frame_id'),
    dataIndex: 'frameId',
    align: 'right',
    textOverflow: 'ellipsis',

    width: 50,
  },
  {
    type: 'string',
    name: t('journalConfig.page_id'),
    dataIndex: 'pageId',
    align: 'right',
    textOverflow: 'ellipsis',
    width: 40,
  },
  {
    type: 'string',
    name: t('journalConfig.msg_type'),
    dataIndex: 'msgTypeName',
    align: 'center',
    width: 180,
  },
  {
    type: 'string',
    name: t('journalConfig.msg_details'),
    dataIndex: 'dataAsString',
    textOverflow: searchInUsing ? 'clip' : 'ellipsis',
    wrap: searchInUsing,
    flex: 1,
  },
];

export const getStrategyColumns = (): AntTableColumn[] => [
  {
    title: t('journalConfig.strategy_id'),
    dataIndex: 'sessionName',
    align: 'center',
    fixed: 'right',
  },
  {
    title: t('journalConfig.begin_time'),
    dataIndex: 'beginTimeResolved',
    align: 'center',
    fixed: 'right',
  },
  {
    title: t('journalConfig.status'),
    dataIndex: 'status',
    align: 'center',
    fixed: 'right',
  },
];

export interface ChartDataCustomInfo {
  tableRowId: string;
  time: bigint;
  msgTypeName: string;
  orderId?: bigint;
}

interface EChartsTooltipPositionParams {
  dataIndex: number;
}

type Point = [number, number];
type RectSize = {
  contentSize: [number, number];
  viewSize: [number, number];
};

export type PosFun = (
  point: Point,
  params: EChartsTooltipPositionParams[],
  dom: HTMLElement,
  rect: DOMRect,
  size: RectSize,
) => string;

export interface SeriesData {
  value: (string | number | bigint)[];
  customInfo?: ChartDataCustomInfo;
  symbolRotate?: number;
  symbolOffset?: (string | number)[];
  symbolSize?: number;
  itemStyle?: ItemStyle;
  showSymbol?: boolean;
  tooltip?: {
    position: string | PosFun;
    formatter: string;
    trigger?: string;
  };
  emphasis?: {
    itemStyle?: {
      color: string;
    };
    showSymbol?: boolean;
    symbol?: string;
    symbolSize?: number;
  };
  blur?: {
    showSymbol?: boolean;
  };
  lineStyle?: {
    silent?: boolean;
  };
  shadowColor?: string;
  shadowBlur?: number;
  label?: {
    show: boolean;
    position: string;
    color: string;
    formatter: () => string;
  };
}

export interface ItemStyle {
  color?: string;
  shadowBlur?: number;
  shadowColor?: string;
}

export const getChartOption = () => {
  return {
    tooltip: {
      trigger: 'item',
      backgroundColor: '#1d1d1d',
      borderColor: '#424242',
      textStyle: {
        color: '#ffffffd9',
      },
    },
    legend: {
      inactiveColor: '#434343',
      textStyle: {
        color: '#FAAD14',
      },
      data: [
        {
          name: t('journalConfig.quote_legend'),
          icon: straightLine,
        },
        {
          name: t('journalConfig.order_input_legend'),
        },
        {
          name: t('journalConfig.order_legend'),
        },
        {
          name: t('journalConfig.cancel_order_legend'),
        },
      ],
    },
    xAxis: {
      type: 'category',
      data: [] as string[],
      axisLabel: {
        formatter: function (value) {
          return value;
        },
      },
    },
    yAxis: {
      type: 'value',
      splitLine: {
        lineStyle: {
          color: '#434343',
        },
      },
      min: 0 as string | number,
      max: 10 as string | number,
      interval: 2,
    },
    dataZoom: [
      {
        type: 'slider',
        xAxisIndex: 0,
        start: 0,
        end: 100,
        labelFormatter: function (params: string) {
          return params;
        },
        textStyle: {
          color: '#ffffffd9',
        },
      },
      {
        type: 'inside',
        xAxisIndex: 0,
        start: 0,
        end: 100,
        labelFormatter: function (params: string) {
          return params;
        },
        textStyle: {
          color: '#ffffffd9',
        },
      },
    ],
    series: [
      {
        name: t('journalConfig.quote_legend'),
        type: 'line',
        data: [] as SeriesData[],
        symbol: circleSvg,
        symbolSize: 10,
        showSymbol: false,
        itemStyle: {
          color: '#0F6DA6',
        },
        lineStyle: {
          color: '#0F6DA6',
        },
        silent: true,
        zlevel: 0,
      },
      {
        name: t('journalConfig.order_input_legend'),
        type: 'scatter',
        symbolSize: 8,
        data: [] as SeriesData[],
        legendHoverLink: false,
        symbolKeepAspect: false,
        symbolOffset: [],
        symbol: triangleSvg,
        zlevel: 2,
      },
      {
        name: t('journalConfig.order_legend'),
        type: 'scatter',
        symbolSize: 12,
        data: [] as SeriesData[],
        legendHoverLink: false,
        symbolKeepAspect: true,
        symbolOffset: [],
        symbol: arrowSvg,
        zlevel: 3,
        itemStyle: {
          borderColor: 'transparent',
          borderWidth: 10,
        },
      },
      {
        name: t('journalConfig.cancel_order_legend'),
        type: 'scatter',
        symbolSize: 10,
        data: [] as SeriesData[],
        legendHoverLink: false,
        symbolKeepAspect: true,
        symbolOffset: [],
        symbol: forkSvg,
        itemStyle: {
          color: '#73F3F6',
        },
        zlevel: 3,
      },
      {
        name: t('journalConfig.quote_legend'),
        type: 'scatter',
        data: [] as SeriesData[],
        symbol: circleSvg,
        showSymbol: true,
        symbolSize: 10,
        legendHoverLink: false,
        symbolKeepAspect: false,
        symbolOffset: [],
        itemStyle: {
          color: 'transparent',
        },
        emphasis: {
          symbolSize: 10,
          itemStyle: {
            color: '#0F6DA6',
          },
        },
        zlevel: 1,
      },
    ],
    grid: {
      left: '40px',
      right: '80px',
      containLabel: true,
    },
  };
};

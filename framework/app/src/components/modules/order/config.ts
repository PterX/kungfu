import { DealTradingTableHooks } from '@kungfu-trader/kungfu-js-api/hooks/dealTradingTableHook';
import { UnfinishedOrderStatus } from '@kungfu-trader/kungfu-js-api/config/tradingConfig';
import { defaultColorMap } from '@kungfu-trader/kungfu-js-api/config/systemConfig';
import {
  vTableSorter,
  dealKfDecimalPrecision,
} from '@kungfu-trader/kungfu-js-api/utils/commonUtils';
import { VTable } from '@kungfu-trader/kungfu-app/src/renderer/assets/configs/vTable';
import {
  dealOffset,
  dealSide,
  getAccountIdStyle,
  getPrecisionByInstrumentType,
} from '@kungfu-trader/kungfu-js-api/utils/tradingUtils';
import { dealKfTime } from '@kungfu-trader/kungfu-js-api/kungfu';
import { useLanguage } from '@kungfu-trader/kungfu-js-api/language';
const { t } = useLanguage();

export const defaultColumnsWidth: Record<string, number> = {
  insert_time: 160,
  instrument_id: 140,
  side: 80,
  offset: 60,
  limit_price_resolved: 120,
  avg_price_resolved: 120,
  volume_left: 120,
  status_resolved: 120,
  actions: 60,
  latency_system: 160,
  latency_network: 160,
  dest_uname: 300,
  source_uname: 300,
};

const columnConfig = (field, title, width, options = {}) => ({
  field,
  title,
  width,
  disableSelect: true,
  ...options,
});

export const getDefaultColumns = (
  kfLocation: KungfuApi.KfLocation,
  isHistory: boolean,
): VTable.ColumnDefine[] => [
  columnConfig(
    'insert_time',
    t('orderConfig.order_time'),
    defaultColumnsWidth.insert_time,
    {
      sort: vTableSorter,
      fieldFormat: (args) => dealKfTime(args.insert_time, true),
    },
  ),
  columnConfig(
    'instrument_id',
    t('orderConfig.instrument_id'),
    defaultColumnsWidth.instrument_id,
  ),
  columnConfig('side', '', defaultColumnsWidth.side, {
    style: {
      color: (args) =>
        defaultColorMap[dealSide(args.dataValue).color || 'default'],
    },
    fieldFormat: (args) => dealSide(args.side).name,
  }),
  columnConfig('offset', '', defaultColumnsWidth.offset, {
    style: {
      color: (args) =>
        defaultColorMap[dealOffset(args.dataValue).color || 'default'],
    },
    fieldFormat: (args) => dealOffset(args.offset).name,
  }),
  columnConfig(
    'limit_price_resolved',
    t('orderConfig.limit_price'),
    defaultColumnsWidth.limit_price_resolved,
    {
      sort: vTableSorter,
      style: { textAlign: 'right' },
      headerStyle: { textAlign: 'right' },
    },
  ),
  columnConfig(
    'avg_price_resolved',
    t('orderConfig.avg_price'),
    defaultColumnsWidth.avg_price_resolved,
    {
      sort: vTableSorter,
      style: { textAlign: 'right' },
      headerStyle: { textAlign: 'right' },
    },
  ),
  columnConfig(
    'volume_left',
    `${t('orderConfig.clinch')}/${t('orderConfig.all')}`,
    defaultColumnsWidth.volume_left,
    {
      sort: vTableSorter,
      style: { textAlign: 'right' },
      headerStyle: { textAlign: 'right' },
      fieldFormat: (args) => {
        const precision = getPrecisionByInstrumentType(args.instrument_type);
        return `${dealKfDecimalPrecision(
          args.volume - args.volume_left,
          precision,
        )} / ${dealKfDecimalPrecision(args.volume, precision)}`;
      },
    },
  ),
  columnConfig(
    'status_resolved',
    t('orderConfig.order_status'),
    defaultColumnsWidth.status_resolved,
    {
      style: {
        color: (args) => defaultColorMap[args.dataValue?.color || 'default'],
      },
      fieldFormat: (args) => args.status_resolved?.name,
    },
  ),
  ...(!isHistory
    ? [
        columnConfig('actions', '', defaultColumnsWidth.actions, {
          style: {
            color: defaultColorMap.red,
            cursor: 'pointer',
          },
          fieldFormat: (args) =>
            UnfinishedOrderStatus.includes(args.status)
              ? t('orderConfig.cancel_order')
              : '',
        }),
      ]
    : []),
  columnConfig(
    'latency_system',
    t('orderConfig.latency_system'),
    defaultColumnsWidth.latency_system,
    {
      sort: vTableSorter,
    },
  ),
  columnConfig(
    'latency_network',
    t('orderConfig.latency_network'),
    defaultColumnsWidth.latency_network,
    {
      sort: vTableSorter,
    },
  ),
  columnConfig(
    kfLocation.category === 'td' ? 'dest_uname' : 'source_uname',
    kfLocation.category === 'td'
      ? t('orderConfig.dest_uname')
      : t('orderConfig.source_uname'),
    300,
    {
      style: {
        color: (args) => getAccountIdStyle(args.dataValue),
      },
    },
  ),
];

export const getColumnsConfig = (
  columnsWidth: Record<string, number> = defaultColumnsWidth,
  isHistory = false,
) => ({
  insert_time: columnConfig(
    'insert_time',
    t('orderConfig.order_time'),
    columnsWidth.insert_time,
    {
      sort: vTableSorter,
      fieldFormat: (args) => dealKfTime(args.insert_time, true),
    },
  ),
  instrument_id: columnConfig(
    'instrument_id',
    t('orderConfig.instrument_id'),
    columnsWidth.instrument_id,
  ),
  side: columnConfig('side', '', columnsWidth.side, {
    style: {
      color: (args) =>
        defaultColorMap[dealSide(args.dataValue).color || 'default'],
    },
    fieldFormat: (args) => dealSide(args.side).name,
  }),
  offset: columnConfig('offset', '', columnsWidth.offset, {
    style: {
      color: (args) =>
        defaultColorMap[dealOffset(args.dataValue).color || 'default'],
    },
    fieldFormat: (args) => dealOffset(args.offset).name,
  }),
  limit_price_resolved: columnConfig(
    'limit_price_resolved',
    t('orderConfig.limit_price'),
    columnsWidth.limit_price_resolved,
    {
      sort: vTableSorter,
      style: { textAlign: 'right' },
      headerStyle: { textAlign: 'right' },
    },
  ),
  avg_price_resolved: columnConfig(
    'avg_price_resolved',
    t('orderConfig.avg_price'),
    columnsWidth.avg_price_resolved,
    {
      sort: vTableSorter,
      style: { textAlign: 'right' },
      headerStyle: { textAlign: 'right' },
    },
  ),
  volume_left: columnConfig(
    'volume_left',
    `${t('orderConfig.clinch')}/${t('orderConfig.all')}`,
    columnsWidth.volume_left,
    {
      sort: vTableSorter,
      style: { textAlign: 'right' },
      headerStyle: { textAlign: 'right' },
      fieldFormat: (args) => {
        const precision = getPrecisionByInstrumentType(args.instrument_type);
        return `${dealKfDecimalPrecision(
          args.volume - args.volume_left,
          precision,
        )} / ${dealKfDecimalPrecision(args.volume, precision)}`;
      },
    },
  ),
  status_resolved: columnConfig(
    'status_resolved',
    t('orderConfig.order_status'),
    columnsWidth.status_resolved,
    {
      style: {
        color: (args) => defaultColorMap[args.dataValue?.color || 'default'],
      },
      fieldFormat: (args) => args.status_resolved?.name,
    },
  ),
  actions: !isHistory
    ? columnConfig('actions', '', columnsWidth.actions, {
        style: {
          color: defaultColorMap.red,
          cursor: 'pointer',
        },
        fieldFormat: (args) =>
          UnfinishedOrderStatus.includes(args.status)
            ? t('orderConfig.cancel_order')
            : '',
      })
    : undefined,
  latency_system: columnConfig(
    'latency_system',
    t('orderConfig.latency_system'),
    columnsWidth.latency_system,
    {
      sort: vTableSorter,
    },
  ),
  latency_network: columnConfig(
    'latency_network',
    t('orderConfig.latency_network'),
    columnsWidth.latency_network,
    {
      sort: vTableSorter,
    },
  ),
  dest_uname: columnConfig(
    'dest_uname',
    t('orderConfig.dest_uname'),
    columnsWidth.dest_uname,
    {
      style: {
        color: (args) => getAccountIdStyle(args.dataValue),
      },
    },
  ),
  source_uname: columnConfig(
    'source_uname',
    t('orderConfig.source_uname'),
    columnsWidth.source_uname,
    {
      style: {
        color: (args) => getAccountIdStyle(args.dataValue),
      },
    },
  ),
});

export const getColumns = ({
  kfLocation,
  isHistory,
  boardResizeConfig,
}: {
  kfLocation: KungfuApi.KfLocation;
  isHistory: boolean;
  boardResizeConfig: ColumnsSetting | null;
}): VTable.ColumnDefine[] => {
  let columnDefineList: VTable.ColumnDefine[] = [];

  if (!boardResizeConfig) {
    columnDefineList = getDefaultColumns(kfLocation, isHistory);
  } else {
    const columnsConfig = getColumnsConfig(
      boardResizeConfig.columnsWidth,
      isHistory,
    );
    const columns = boardResizeConfig.fields
      .map((field) => columnsConfig[field])
      .filter(Boolean) as VTable.ColumnDefine[];
    columnDefineList =
      columns.length > 0 ? columns : getDefaultColumns(kfLocation, isHistory);
  }

  return (
    globalThis.HookKeeper.getHooks().dealTradingTable as DealTradingTableHooks
  )
    .trigger(kfLocation, 'order')
    .getColumns<VTable.ColumnDefine>(columnDefineList);
};

export const statisColums: KfTradingDataTableHeaderConfig[] = [
  {
    name: t('tradingConfig.instrument'),
    dataIndex: 'instrumentId_exchangeId',
  },
  {
    name: '',
    dataIndex: 'sideName',
    width: 80,
  },
  {
    name: '',
    dataIndex: 'offsetName',
    width: 60,
  },
  {
    name: t('orderConfig.mean'),
    dataIndex: 'mean',
  },
  {
    name: t('orderConfig.max'),
    dataIndex: 'max',
  },
  {
    name: t('orderConfig.min'),
    dataIndex: 'min',
  },
  {
    name: `${t('orderConfig.volume')}(${t('orderConfig.completed')}/${t(
      'orderConfig.all',
    )})`,
    dataIndex: 'volume',
  },
];

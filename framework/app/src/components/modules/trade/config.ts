import { DealTradingTableHooks } from '@kungfu-trader/kungfu-js-api/hooks/dealTradingTableHook';
import { vTableSorter } from '@kungfu-trader/kungfu-js-api/utils/commonUtils';
import { defaultColorMap } from '@kungfu-trader/kungfu-js-api/config/systemConfig';
import { VTable } from '@kungfu-trader/kungfu-app/src/renderer/assets/configs/vTable';
import {
  dealOffset,
  dealSide,
  getAccountIdStyle,
} from '@kungfu-trader/kungfu-js-api/utils/tradingUtils';
import { dealKfTime } from '@kungfu-trader/kungfu-js-api/kungfu';
import { useLanguage } from '@kungfu-trader/kungfu-js-api/language';
const { t } = useLanguage();

export const defaultColumnsWidth: Record<string, number> = {
  trade_time: 160,
  instrument_id: 140,
  side: 80,
  offset: 50,
  price_resolved: 120,
  volume: 120,
  dest_uname: 300,
  source_uname: 300,
};

const columnConfig = (
  field: string,
  title: string,
  width: number,
  options = {},
) => ({
  field,
  title,
  width,
  disableSelect: true,
  ...options,
});

export const getDefaultColumns = (
  kfLocation: KungfuApi.KfLocation,
): VTable.ColumnDefine[] => [
  columnConfig(
    'trade_time',
    t('tradeConfig.trade_time_resolved'),
    defaultColumnsWidth.trade_time,
    {
      sort: vTableSorter,
      fieldFormat: (args) => dealKfTime(args.trade_time, true),
    },
  ),
  columnConfig(
    'instrument_id',
    t('tradeConfig.instrument_id'),
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
    'price_resolved',
    t('tradeConfig.price'),
    defaultColumnsWidth.price_resolved,
    {
      sort: vTableSorter,
      style: { textAlign: 'right' },
      headerStyle: { textAlign: 'right' },
    },
  ),
  columnConfig('volume', t('tradeConfig.volume'), defaultColumnsWidth.volume, {
    sort: vTableSorter,
    style: { textAlign: 'right' },
    headerStyle: { textAlign: 'right' },
  }),
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
) => ({
  trade_time: columnConfig(
    'trade_time',
    t('tradeConfig.trade_time_resolved'),
    columnsWidth.trade_time || defaultColumnsWidth.trade_time,
    {
      sort: true,
      fieldFormat: (args) => dealKfTime(args.trade_time, true),
    },
  ),
  instrument_id: columnConfig(
    'instrument_id',
    t('tradeConfig.instrument_id'),
    columnsWidth.instrument_id || defaultColumnsWidth.instrument_id,
  ),
  side: columnConfig(
    'side',
    '',
    columnsWidth.side || defaultColumnsWidth.side,
    {
      style: {
        color: (args) =>
          defaultColorMap[dealSide(args.dataValue).color || 'default'],
      },
      fieldFormat: (args) => dealSide(args.side).name,
    },
  ),
  offset: columnConfig(
    'offset',
    '',
    columnsWidth.offset || defaultColumnsWidth.offset,
    {
      style: {
        color: (args) =>
          defaultColorMap[dealOffset(args.dataValue).color || 'default'],
      },
      fieldFormat: (args) => dealOffset(args.offset).name,
    },
  ),
  price_resolved: columnConfig(
    'price_resolved',
    t('tradeConfig.price'),
    columnsWidth.price_resolved || defaultColumnsWidth.price_resolved,
    {
      sort: true,
      style: { textAlign: 'right' },
      headerStyle: { textAlign: 'right' },
    },
  ),
  volume: columnConfig(
    'volume',
    t('tradeConfig.volume'),
    columnsWidth.volume || defaultColumnsWidth.volume,
    {
      sort: true,
      style: { textAlign: 'right' },
      headerStyle: { textAlign: 'right' },
    },
  ),
  dest_uname: columnConfig(
    'dest_uname',
    t('orderConfig.dest_uname'),
    columnsWidth.dest_uname || defaultColumnsWidth.dest_uname,
    {
      style: {
        color: (args) => getAccountIdStyle(args.dataValue),
      },
    },
  ),
  source_uname: columnConfig(
    'source_uname',
    t('orderConfig.source_uname'),
    columnsWidth.source_uname || defaultColumnsWidth.source_uname,
    {
      style: {
        color: (args) => getAccountIdStyle(args.dataValue),
      },
    },
  ),
});

export const getColumns = ({
  kfLocation,
  boardResizeConfig,
}: {
  kfLocation: KungfuApi.KfLocation;
  boardResizeConfig: ColumnsSetting | null;
}): VTable.ColumnDefine[] => {
  const columnDefineList = boardResizeConfig
    ? boardResizeConfig.fields.map(
        (field) => getColumnsConfig(boardResizeConfig.columnsWidth)[field],
      )
    : getDefaultColumns(kfLocation);

  return (
    globalThis.HookKeeper.getHooks().dealTradingTable as DealTradingTableHooks
  )
    .trigger(kfLocation, 'trade')
    .getColumns<VTable.ColumnDefine>(columnDefineList);
};

export const statisColums: KfTradingDataTableHeaderConfig[] = [
  {
    name: t('tradeConfig.instrument_id'),
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
    name: t('tradeConfig.mean_price'),
    dataIndex: 'mean',
  },
  {
    name: t('tradeConfig.max_price'),
    dataIndex: 'max',
  },
  {
    name: t('tradeConfig.min_price'),
    dataIndex: 'min',
  },
  {
    name: t('tradeConfig.volume'),
    dataIndex: 'volume',
  },
];

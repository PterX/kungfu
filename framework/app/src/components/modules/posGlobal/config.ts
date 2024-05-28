import { LedgerCategoryEnum } from '@kungfu-trader/kungfu-js-api/typings/enums';
import VueI18n from '@kungfu-trader/kungfu-js-api/language';
import { vTableSorter } from '@kungfu-trader/kungfu-js-api/utils/commonUtils';
import { defaultColorMap } from '@kungfu-trader/kungfu-js-api/config/systemConfig';
import { useQuote } from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/actionsUtils';

import { DealTradingDataGetter } from '@kungfu-trader/kungfu-js-api/hooks/dealTradingDataHook';
import { getTradingDataSortKey } from '@kungfu-trader/kungfu-js-api/utils/busiUtils';
import { VTable } from '@kungfu-trader/kungfu-app/src/renderer/assets/configs/vTable';
import { dealDirection } from '@kungfu-trader/kungfu-js-api/utils/tradingUtils';

const { t } = VueI18n.global;

const { getPositionLastPrice } = useQuote();
export { getPositionLastPrice };

export const defaultColumnsWidth: Record<string, number> = {
  instrument_id: 190,
  direction: 50,
  static_yesterday: 110,
  open_volume: 110,
  close_volume: 110,
  yesterday_volume: 110,
  today_volume: 110,
  volume: 110,
  avg_open_price_resolved: 110,
  last_price_resolved: 110,
  unrealized_pnl_resolved: 110,
};

const columnConfig = (field, title, width, options = {}) => ({
  field,
  title,
  width,
  disableSelect: true,
  ...options,
});

export const getDefaultColumns = (): VTable.ColumnDefine[] => [
  columnConfig(
    'instrument_id',
    t('posGlobalConfig.instrument_id'),
    defaultColumnsWidth.instrument_id,
    {
      sort: vTableSorter,
    },
  ),
  columnConfig('direction', '', defaultColumnsWidth.direction, {
    style: {
      color: (args) =>
        defaultColorMap[dealDirection(args.dataValue).color || 'default'],
    },
    fieldFormat: (args) => dealDirection(args.direction).name,
  }),
  columnConfig(
    'static_yesterday',
    t('posGlobalConfig.static_yesterday'),
    defaultColumnsWidth.static_yesterday,
    {
      sort: vTableSorter,
      style: { textAlign: 'right' },
      headerStyle: { textAlign: 'right' },
    },
  ),
  columnConfig(
    'open_volume',
    t('posGlobalConfig.open_volume'),
    defaultColumnsWidth.open_volume,
    {
      sort: vTableSorter,
      style: { textAlign: 'right' },
      headerStyle: { textAlign: 'right' },
    },
  ),
  columnConfig(
    'close_volume',
    t('posGlobalConfig.close_volume'),
    defaultColumnsWidth.close_volume,
    {
      sort: vTableSorter,
      style: { textAlign: 'right' },
      headerStyle: { textAlign: 'right' },
    },
  ),
  columnConfig(
    'yesterday_volume',
    t('posGlobalConfig.yesterday_volume'),
    defaultColumnsWidth.yesterday_volume,
    {
      sort: vTableSorter,
      style: { textAlign: 'right' },
      headerStyle: { textAlign: 'right' },
    },
  ),
  columnConfig(
    'today_volume',
    t('posGlobalConfig.today_volume'),
    defaultColumnsWidth.today_volume,
    {
      sort: vTableSorter,
      style: { textAlign: 'right' },
      headerStyle: { textAlign: 'right' },
    },
  ),
  columnConfig(
    'volume',
    t('posGlobalConfig.sum_volume'),
    defaultColumnsWidth.volume,
    {
      sort: vTableSorter,
      style: { textAlign: 'right' },
      headerStyle: { textAlign: 'right' },
    },
  ),
  columnConfig(
    'avg_open_price_resolved',
    t('posGlobalConfig.avg_open_price'),
    defaultColumnsWidth.avg_open_price_resolved,
    {
      sort: vTableSorter,
      style: { textAlign: 'right' },
      headerStyle: { textAlign: 'right' },
    },
  ),
  columnConfig(
    'last_price_resolved',
    t('posGlobalConfig.last_price'),
    defaultColumnsWidth.last_price_resolved,
    {
      sort: vTableSorter,
      style: { textAlign: 'right' },
      headerStyle: { textAlign: 'right' },
      fieldFormat: (args) =>
        getPositionLastPrice(args, 'last_price_resolved') ?? '--',
    },
  ),
  columnConfig(
    'unrealized_pnl_resolved',
    t('posGlobalConfig.unrealized_pnl'),
    defaultColumnsWidth.unrealized_pnl_resolved,
    {
      sort: vTableSorter,
      style: {
        textAlign: 'right',
        color: (args) => {
          if (!Number(args.dataValue)) return defaultColorMap['text'];
          return +args.dataValue > 0
            ? defaultColorMap['red']
            : defaultColorMap['green'];
        },
      },
      headerStyle: { textAlign: 'right' },
    },
  ),
];

export const getColumnsConfig = (
  columnsWidth: Record<string, number> = defaultColumnsWidth,
): Record<string, VTable.ColumnDefine> => ({
  instrument_id: columnConfig(
    'instrument_id',
    t('posGlobalConfig.instrument_id'),
    columnsWidth.instrument_id,
    {
      sort: vTableSorter,
    },
  ),
  direction: columnConfig('direction', '', columnsWidth.direction, {
    style: {
      color: (args) =>
        defaultColorMap[dealDirection(args.dataValue).color || 'default'],
    },
    fieldFormat: (args) => dealDirection(args.direction).name,
  }),
  static_yesterday: columnConfig(
    'static_yesterday',
    t('posGlobalConfig.static_yesterday'),
    columnsWidth.static_yesterday,
    {
      sort: vTableSorter,
      style: { textAlign: 'right' },
      headerStyle: { textAlign: 'right' },
    },
  ),
  open_volume: columnConfig(
    'open_volume',
    t('posGlobalConfig.open_volume'),
    columnsWidth.open_volume,
    {
      sort: vTableSorter,
      style: { textAlign: 'right' },
      headerStyle: { textAlign: 'right' },
    },
  ),
  close_volume: columnConfig(
    'close_volume',
    t('posGlobalConfig.close_volume'),
    columnsWidth.close_volume,
    {
      sort: vTableSorter,
      style: { textAlign: 'right' },
      headerStyle: { textAlign: 'right' },
    },
  ),
  yesterday_volume: columnConfig(
    'yesterday_volume',
    t('posGlobalConfig.yesterday_volume'),
    columnsWidth.yesterday_volume,
    {
      sort: vTableSorter,
      style: { textAlign: 'right' },
      headerStyle: { textAlign: 'right' },
    },
  ),
  today_volume: columnConfig(
    'today_volume',
    t('posGlobalConfig.today_volume'),
    columnsWidth.today_volume,
    {
      sort: vTableSorter,
      style: { textAlign: 'right' },
      headerStyle: { textAlign: 'right' },
    },
  ),
  volume: columnConfig(
    'volume',
    t('posGlobalConfig.sum_volume'),
    columnsWidth.volume,
    {
      sort: vTableSorter,
      style: { textAlign: 'right' },
      headerStyle: { textAlign: 'right' },
    },
  ),
  avg_open_price_resolved: columnConfig(
    'avg_open_price_resolved',
    t('posGlobalConfig.avg_open_price'),
    columnsWidth.avg_open_price_resolved,
    {
      sort: vTableSorter,
      style: { textAlign: 'right' },
      headerStyle: { textAlign: 'right' },
    },
  ),
  last_price_resolved: columnConfig(
    'last_price_resolved',
    t('posGlobalConfig.last_price'),
    columnsWidth.last_price_resolved,
    {
      sort: vTableSorter,
      style: { textAlign: 'right' },
      headerStyle: { textAlign: 'right' },
      fieldFormat: (args) =>
        getPositionLastPrice(args, 'last_price_resolved') ?? '--',
    },
  ),
  unrealized_pnl_resolved: columnConfig(
    'unrealized_pnl_resolved',
    t('posGlobalConfig.unrealized_pnl'),
    columnsWidth.unrealized_pnl_resolved,
    {
      sort: vTableSorter,
      style: {
        textAlign: 'right',
        color: (args) => {
          if (!Number(args.dataValue)) return defaultColorMap['text'];
          return +args.dataValue > 0
            ? defaultColorMap['red']
            : defaultColorMap['green'];
        },
      },
      headerStyle: { textAlign: 'right' },
    },
  ),
});

export const getColumns = ({
  boardResizeConfig,
}: {
  boardResizeConfig: ColumnsSetting | null;
}): VTable.ColumnDefine[] => {
  let columnDefineList: VTable.ColumnDefine[] = [];

  if (!boardResizeConfig) {
    columnDefineList = getDefaultColumns();
  } else {
    columnDefineList = boardResizeConfig.fields
      .map((field) => getColumnsConfig(boardResizeConfig.columnsWidth)[field])
      .filter(Boolean) as VTable.ColumnDefine[];
  }

  return columnDefineList;
};

const orderSortKey = getTradingDataSortKey('Order');
const tradeSortKey = getTradingDataSortKey('Trade');
const positionSortKey = getTradingDataSortKey('Position');
export const categoryRegisterConfig: DealTradingDataGetter = {
  category: 'globalPos',
  commonData: {
    name: t('PosGlobal'),
    color: 'pink',
  },
  order: {
    getter(_watcher, orders, kfLocation) {
      const { group, name } = kfLocation;
      return orders
        .filter('exchange_id', group)
        .filter('instrument_id', name)
        .sort(orderSortKey);
    },
  },
  trade: {
    getter(_watcher, trades, kfLocation) {
      const { group, name } = kfLocation;
      return trades
        .filter('exchange_id', group)
        .filter('instrument_id', name)
        .sort(tradeSortKey);
    },
  },
  position: {
    getter(_watcher, position, kfLocation) {
      const { group, name, direction } =
        kfLocation as KungfuApi.KfExtraLocation;
      return position
        .nofilter('volume', 0)
        .filter('ledger_category', LedgerCategoryEnum.td)
        .filter('exchange_id', group)
        .filter('instrument_id', name)
        .filter('direction', direction)
        .sort(positionSortKey)
        .reverse();
    },
  },
};

import { DealTradingTableHooks } from '@kungfu-trader/kungfu-js-api/hooks/dealTradingTableHook';
import { isTd } from '@kungfu-trader/kungfu-js-api/utils/busiUtils';
import { vTableSorter } from '@kungfu-trader/kungfu-js-api/utils/commonUtils';
import { defaultColorMap } from '@kungfu-trader/kungfu-js-api/config/systemConfig';

import { useQuote } from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/actionsUtils';
import { VTable } from '@kungfu-trader/kungfu-app/src/renderer/assets/configs/vTable';
import { dealDirection } from '@kungfu-trader/kungfu-js-api/utils/tradingUtils';
import VueI18n from '@kungfu-trader/kungfu-js-api/language';
const { t } = VueI18n.global;

const { getPositionLastPrice } = useQuote();
export { getPositionLastPrice };

export const defaultColumnsWidth: Record<string, number> = {
  instrument_id_resolved: 190,
  account_id_resolved: 120,
  direction: 50,
  static_yesterday: 110,
  open_volume: 110,
  close_volume: 110,
  yesterday_volume: 110,
  today_volume: 110,
  volume: 110,
  frozen_total: 110,
  closable_volume: 110,
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

export const getDefaultColumns = (
  kfLocation: KungfuApi.KfLocation,
): VTable.ColumnDefine[] => {
  return [
    columnConfig(
      'instrument_id_resolved',
      t('posGlobalConfig.instrument_id'),
      defaultColumnsWidth.instrument_id_resolved,
      {
        sort: vTableSorter,
      },
    ),
    ...(isTd(kfLocation.category)
      ? []
      : [
          columnConfig(
            'account_id_resolved',
            t('posGlobalConfig.account_id_resolved'),
            defaultColumnsWidth.account_id_resolved,
            {
              sort: vTableSorter,
            },
          ),
        ]),
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
      'frozen_total',
      t('posGlobalConfig.frozen_volume'),
      defaultColumnsWidth.frozen_total,
      {
        sort: vTableSorter,
        style: { textAlign: 'right' },
        headerStyle: { textAlign: 'right' },
      },
    ),
    columnConfig(
      'closable_volume',
      t('posGlobalConfig.closable_volume'),
      defaultColumnsWidth.closable_volume,
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
};

export const getColumnsConfig = (
  columnsWidth: Record<string, number> = defaultColumnsWidth,
): Record<string, VTable.ColumnDefine> => ({
  instrument_id_resolved: columnConfig(
    'instrument_id_resolved',
    t('posGlobalConfig.instrument_id'),
    columnsWidth.instrument_id_resolved,
    {
      sort: vTableSorter,
    },
  ),
  account_id_resolved: columnConfig(
    'account_id_resolved',
    t('posGlobalConfig.account_id_resolved'),
    columnsWidth.account_id_resolved,
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
  frozen_total: columnConfig(
    'frozen_total',
    t('posGlobalConfig.frozen_volume'),
    columnsWidth.frozen_total,
    {
      sort: vTableSorter,
      style: { textAlign: 'right' },
      headerStyle: { textAlign: 'right' },
    },
  ),
  closable_volume: columnConfig(
    'closable_volume',
    t('posGlobalConfig.closable_volume'),
    columnsWidth.closable_volume,
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
  kfLocation,
  boardResizeConfig,
}: {
  kfLocation: KungfuApi.KfLocation;
  boardResizeConfig: ColumnsSetting | null;
}): VTable.ColumnDefine[] => {
  let columnDefineList: VTable.ColumnDefine[] = [];

  if (!boardResizeConfig) {
    columnDefineList = getDefaultColumns(kfLocation);
  } else {
    columnDefineList = boardResizeConfig.fields
      .map((field) => getColumnsConfig(boardResizeConfig.columnsWidth)[field])
      .filter(Boolean) as VTable.ColumnDefine[];
  }

  return (
    globalThis.HookKeeper.getHooks().dealTradingTable as DealTradingTableHooks
  )
    .trigger(kfLocation, 'position')
    .getColumns<VTable.ColumnDefine>(columnDefineList);
};

import { DealTradingTableHooks } from '@kungfu-trader/kungfu-js-api/hooks/dealTradingTableHook';
import {
  isTdStrategyCategory,
  vTableSorter,
} from '@kungfu-trader/kungfu-js-api/utils/commonUtils';
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

export const getColumns = (
  kfLocation: KungfuApi.KfLocation,
): VTable.ColumnDefine[] =>
  (globalThis.HookKeeper.getHooks().dealTradingTable as DealTradingTableHooks)
    .trigger(kfLocation, 'trade')
    .getColumns<VTable.ColumnDefine>([
      {
        field: 'trade_time',
        title: t('tradeConfig.trade_time_resolved'),
        width: 160,
        sort: vTableSorter,
        fieldFormat: (args) => {
          return dealKfTime(args.trade_time, true);
        },
      },
      {
        field: 'instrument_id',
        title: t('tradeConfig.instrument_id'),
        width: 80,
      },
      {
        field: 'side',
        title: '',
        width: 60,
        style: {
          color: (args) => {
            return defaultColorMap[dealSide(args.dataValue).color || 'default'];
          },
        },
        fieldFormat: (args) => {
          return dealSide(args.side).name;
        },
      },
      {
        field: 'offset',
        title: '',
        width: 60,
        style: {
          color: (args) => {
            return defaultColorMap[
              dealOffset(args.dataValue).color || 'default'
            ];
          },
        },
        fieldFormat: (args) => {
          return dealOffset(args.offset).name;
        },
      },
      {
        field: 'price_resolved',
        title: t('tradeConfig.price'),
        width: 120,
        style: {
          textAlign: 'right',
        },
        headerStyle: {
          textAlign: 'right',
        },
        sort: vTableSorter,
      },
      {
        field: 'volume',
        title: t('tradeConfig.volume'),
        width: 120,
        style: {
          textAlign: 'right',
        },
        headerStyle: {
          textAlign: 'right',
        },
        sort: vTableSorter,
      },
      {
        field: kfLocation.category === 'td' ? 'dest_uname' : 'source_uname',
        title:
          kfLocation.category === 'td'
            ? t('orderConfig.dest_uname')
            : t('orderConfig.source_uname'),
        width: 300,
        style: {
          color: (args) => {
            return getAccountIdStyle(args.dataValue);
          },
        },
      },
      ...(isTdStrategyCategory(kfLocation.category)
        ? []
        : [
            {
              field: 'dest_uname',
              title: t('orderConfig.dest_uname'),
              width: 300,
              style: {
                color: (args) => {
                  return getAccountIdStyle(args.dataValue);
                },
              },
            },
          ]),
    ]);

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

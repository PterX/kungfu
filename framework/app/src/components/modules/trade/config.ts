import { DealTradingTableHooks } from '@kungfu-trader/kungfu-js-api/hooks/dealTradingTableHook';
import {
  isTdStrategyCategory,
  sorter,
} from '@kungfu-trader/kungfu-js-api/utils/commonUtils';
import { defaultColorMap } from '@kungfu-trader/kungfu-js-api/config/systemConfig';
import { VTable } from '@kungfu-trader/kungfu-app/src/renderer/assets/configs/vTable';
import {
  dealOffset,
  dealSide,
  getAccountIdStyle,
} from '@kungfu-trader/kungfu-js-api/utils/tradingUtils';

import VueI18n from '@kungfu-trader/kungfu-js-api/language';
const { t } = VueI18n.global;

export const getColumns = (
  kfLocation: KungfuApi.KfLocation,
  isHistory = false,
): VTable.ColumnDefine[] =>
  (globalThis.HookKeeper.getHooks().dealTradingTable as DealTradingTableHooks)
    .trigger(kfLocation, 'trade')
    .getColumns<VTable.ColumnDefine>([
      {
        field: 'trade_time_resolved',
        title: t('tradeConfig.trade_time_resolved'),
        width: isHistory ? 160 : 120,
        sort: sorter,
      },
      {
        field: 'kf_time_resovlved',
        title: t('tradeConfig.kf_time_resolved'),
        width: isHistory ? 160 : 120,
        sort: sorter,
      },
      {
        field: 'instrument_id',
        title: t('tradeConfig.instrument_id'),
        width: 120,
        sort: sorter,
      },
      {
        field: 'side',
        title: '',
        width: 100,
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
        sort: sorter,
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
        sort: sorter,
      },
      {
        field: 'latency_trade',
        title: t('tradeConfig.latency_trade'),
        width: 160,
        style: {
          textAlign: 'right',
        },
        headerStyle: {
          textAlign: 'right',
        },
        sort: sorter,
      },
      {
        field: kfLocation.category === 'td' ? 'dest_uname' : 'source_uname',
        title:
          kfLocation.category === 'td'
            ? t('orderConfig.dest_uname')
            : t('orderConfig.source_uname'),
        sort: sorter,
        width: 300,
        style: {
          color: (args) => {
            return defaultColorMap[
              getAccountIdStyle(args.dataValue) || 'default'
            ];
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
                  return defaultColorMap[
                    getAccountIdStyle(args.dataValue) || 'default'
                  ];
                },
              },
              sort: sorter,
            },
          ]),
    ]);

export const statisColums: VTable.ColumnDefine[] = [
  {
    field: 'instrumentId_exchangeId',
    title: t('tradingConfig.instrument'),
    width: 120,
  },
  {
    field: 'side',
    title: '',
    width: 100,
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
    width: 80,
    style: {
      color: (args) => {
        return defaultColorMap[dealOffset(args.dataValue).color || 'default'];
      },
    },
    fieldFormat: (args) => {
      return dealOffset(args.offset).name;
    },
  },
  {
    title: t('orderConfig.mean'),
    field: 'mean',
    width: 100,
  },
  {
    title: t('orderConfig.max'),
    field: 'max',
    width: 100,
  },
  {
    title: t('orderConfig.min'),
    field: 'min',
    width: 100,
  },
  {
    title: `${t('orderConfig.volume')}(${t('orderConfig.completed')}/${t(
      'orderConfig.all',
    )})`,
    field: 'volume',
    width: 160,
  },
];

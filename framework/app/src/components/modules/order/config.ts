import { DealTradingTableHooks } from '@kungfu-trader/kungfu-js-api/hooks/dealTradingTableHook';
import { getOrderStatusStyle } from '@kungfu-trader/kungfu-js-api/config/tradingConfig';
import { defaultColorMap } from '@kungfu-trader/kungfu-js-api/config/systemConfig';
import {
  isTdStrategyCategory,
  sorter,
  dealKfDecimalPrecision,
} from '@kungfu-trader/kungfu-js-api/utils/commonUtils';
import { VTable } from '@kungfu-trader/kungfu-app/src/renderer/assets/configs/vTable';
import {
  dealOffset,
  dealSide,
  getAccountIdStyle,
} from '@kungfu-trader/kungfu-js-api/utils/tradingUtils';
import { dealKfTime } from '@kungfu-trader/kungfu-js-api/kungfu';
import VueI18n from '@kungfu-trader/kungfu-js-api/language';
const { t } = VueI18n.global;

export const getColumns = (
  kfLocation: KungfuApi.KfLocation,
  isHistory = false,
): VTable.ColumnDefine[] =>
  (globalThis.HookKeeper.getHooks().dealTradingTable as DealTradingTableHooks)
    .trigger(kfLocation, 'order')
    .getColumns<VTable.ColumnDefine>([
      {
        field: 'update_time',
        title: t('orderConfig.update_time'),
        width: isHistory ? 160 : 120,
        sort: sorter,
        fieldFormat: (args) => {
          return dealKfTime(args.update_time, isHistory);
        },
      },
      {
        field: 'instrument_id',
        title: t('orderConfig.instrument_id'),
        sort: sorter,
        width: 100,
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
        title: t('orderConfig.limit_price'),
        field: 'limit_price_resolved',
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
        field: 'volume_left',
        title: `${t('orderConfig.clinch')}/${t('orderConfig.all')}`,
        width: 120,
        sort: sorter,
        fieldFormat: (args) => {
          return `${dealKfDecimalPrecision(args.volume - args.volume_left)} / ${
            args.volume
          }`;
        },
      },
      {
        field: 'avg_price_resolved',
        title: t('orderConfig.avg_price'),
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
        field: 'status_uname',
        title: t('orderConfig.order_status'),
        width: 120,
        style: {
          color: (args) => {
            return defaultColorMap[
              getOrderStatusStyle(args.dataValue) || 'default'
            ];
          },
        },
        fieldFormat: (args) => {
          return args.status_uname;
        },
      },
      {
        field: 'latency_system',
        title: t('orderConfig.latency_system'),
        width: 110,
        sort: sorter,
      },
      {
        field: 'latency_network',
        title: t('orderConfig.latency_network'),
        width: 110,
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
      ...(isHistory
        ? []
        : [
            {
              field: 'actions',
              title: '',
              width: 120,
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
    width: 60,
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

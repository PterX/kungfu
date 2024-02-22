import { DealTradingTableHooks } from '@kungfu-trader/kungfu-js-api/hooks/dealTradingTableHook';
import {
  getOrderStatusStyle,
  UnfinishedOrderStatus,
} from '@kungfu-trader/kungfu-js-api/config/tradingConfig';
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
        field: 'insert_time',
        title: t('orderConfig.order_time'),
        width: isHistory ? 160 : 120,
        sort: sorter,
        fieldFormat: (args) => {
          return dealKfTime(args.insert_time, isHistory);
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
        width: 80,
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
        width: 50,
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
        field: 'avg_price_resolved',
        title: t('orderConfig.avg_price'),
        width: 100,
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
        width: 144,
        style: {
          textAlign: 'right',
        },
        headerStyle: {
          textAlign: 'right',
        },
        sort: sorter,
        fieldFormat: (args) => {
          return `${dealKfDecimalPrecision(args.volume - args.volume_left)} / ${
            args.volume
          }`;
        },
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
      ...(isHistory
        ? []
        : [
            {
              field: 'actions',
              title: t('orderConfig.actions'),
              width: 60,
              style: {
                color: defaultColorMap.red,
              },
              fieldFormat: (args) => {
                return UnfinishedOrderStatus.includes(args.status)
                  ? t('orderConfig.cancel_order')
                  : '';
              },
            },
          ]),
      // {
      //   field: 'actions',
      //   title: '',
      //   width: 80,
      //   style: {
      //     bgColor: (args) => {
      //       return args.value === t('orderConfig.cancel_order')
      //         ? defaultColorMap.red
      //         : 'transparent';
      //     },
      //     textAlign: 'center',
      //     cursor: 'pointer',
      //   },
      //   headerStyle: {
      //     textAlign: 'center',
      //   },
      //   fieldFormat: (args) => {
      //     return UnfinishedOrderStatus.includes(args.status)
      //       ? t('orderConfig.cancel_order')
      //       : '';
      //   },
      // },
      {
        field: 'latency_system',
        title: t('orderConfig.latency_system'),
        width: 120,
        sort: sorter,
      },
      {
        field: 'latency_network',
        title: t('orderConfig.latency_network'),
        width: 120,
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

// export const statisColums: VTable.ColumnDefine[] = [
//   {
//     field: 'instrumentId_exchangeId',
//     title: t('tradingConfig.instrument'),
//     width: 120,
//   },
//   {
//     field: 'side',
//     title: '',
//     width: 100,
//     style: {
//       color: (args) => {
//         return defaultColorMap[dealSide(args.dataValue).color || 'default'];
//       },
//     },
//     fieldFormat: (args) => {
//       return dealSide(args.side).name;
//     },
//   },
//   {
//     field: 'offset',
//     title: '',
//     width: 60,
//     style: {
//       color: (args) => {
//         return defaultColorMap[dealOffset(args.dataValue).color || 'default'];
//       },
//     },
//     fieldFormat: (args) => {
//       return dealOffset(args.offset).name;
//     },
//   },
//   {
//     title: t('orderConfig.mean'),
//     field: 'mean',
//     width: 100,
//   },
//   {
//     title: t('orderConfig.max'),
//     field: 'max',
//     width: 100,
//   },
//   {
//     title: t('orderConfig.min'),
//     field: 'min',
//     width: 100,
//   },
//   {
//     title: `${t('orderConfig.volume')}(${t('orderConfig.completed')}/${t(
//       'orderConfig.all',
//     )})`,
//     field: 'volume',
//     width: 160,
//   },
// ];

export const statisColums: KfTradingDataTableHeaderConfig[] = [
  {
    name: t('tradingConfig.instrument'),
    dataIndex: 'instrumentId_exchangeId',
  },
  {
    name: '',
    dataIndex: 'sideName',
    width: 40,
  },
  {
    name: '',
    dataIndex: 'offsetName',
    width: 40,
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

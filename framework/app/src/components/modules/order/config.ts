import { DealTradingTableHooks } from '@kungfu-trader/kungfu-js-api/hooks/dealTradingTableHook';
import { UnfinishedOrderStatus } from '@kungfu-trader/kungfu-js-api/config/tradingConfig';
import { defaultColorMap } from '@kungfu-trader/kungfu-js-api/config/systemConfig';
import {
  isTdStrategyCategory,
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
        sort: vTableSorter,
        fieldFormat: (args) => {
          return dealKfTime(args.insert_time, isHistory);
        },
      },
      {
        field: 'instrument_id',
        title: t('orderConfig.instrument_id'),
        width: 140,
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
        sort: vTableSorter,
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

        sort: vTableSorter,
      },
      {
        field: 'volume_left',
        title: `${t('orderConfig.clinch')}/${t('orderConfig.all')}`,
        width: 120,
        style: {
          textAlign: 'right',
        },
        headerStyle: {
          textAlign: 'right',
        },
        sort: vTableSorter,
        fieldFormat: (args) => {
          const precision = getPrecisionByInstrumentType(args.instrument_type);
          return `${dealKfDecimalPrecision(
            args.volume - args.volume_left,
            precision,
          )} / ${dealKfDecimalPrecision(args.volume, precision)}`;
        },
      },
      {
        field: 'status_resolved',
        title: t('orderConfig.order_status'),
        width: 120,
        style: {
          color: (args) => {
            return defaultColorMap[args.dataValue?.color || 'default'];
          },
        },
        fieldFormat: (args) => {
          return args.status_resolved?.name;
        },
      },
      ...(isHistory
        ? []
        : [
            {
              field: 'actions',
              title: '',
              width: 60,
              style: {
                color: defaultColorMap.red,
                cursor: 'pointer',
              },
              fieldFormat: (args) => {
                return UnfinishedOrderStatus.includes(args.status)
                  ? t('orderConfig.cancel_order')
                  : '';
              },
            },
          ]),
      {
        field: 'latency_system',
        title: t('orderConfig.latency_system'),
        width: 160,
        sort: vTableSorter,
      },
      {
        field: 'latency_network',
        title: t('orderConfig.latency_network'),
        width: 160,
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

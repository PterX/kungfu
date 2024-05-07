import { DealTradingTableHooks } from '@kungfu-trader/kungfu-js-api/hooks/dealTradingTableHook';
import {
  isTdStrategyCategory,
  buildTableColumnSorterWithStrike,
} from '@kungfu-trader/kungfu-js-api/utils/commonUtils';
import VueI18n from '@kungfu-trader/kungfu-js-api/language';
const { t } = VueI18n.global;

const buildSorter =
  (dataIndex: keyof KungfuApi.TradeResolved) =>
  (a: KungfuApi.TradeResolved, b: KungfuApi.TradeResolved) =>
    (+Number(a[dataIndex]) || 0) - (+Number(b[dataIndex]) || 0);

const buildStrSorter =
  (dataIndex: keyof KungfuApi.TradeResolved) =>
  (a: KungfuApi.TradeResolved, b: KungfuApi.TradeResolved) =>
    a[dataIndex].toString().localeCompare(b[dataIndex].toString());

export const getColumns = (
  kfLocation: KungfuApi.KfLocation,
): KfTradingDataTableHeaderConfig[] =>
  (globalThis.HookKeeper.getHooks().dealTradingTable as DealTradingTableHooks)
    .trigger(kfLocation, 'trade')
    .getColumns<KfTradingDataTableHeaderConfig>([
      {
        type: 'string',
        name: t('tradeConfig.trade_time_resolved'),
        dataIndex: 'trade_time_resolved',
        width: 160,
        sorter: buildSorter('trade_time'),
      },
      {
        type: 'string',
        name: t('tradeConfig.kf_time_resolved'),
        dataIndex: 'kf_time_resovlved',
        width: 160,
        sorter: buildSorter('trade_time'),
      },
      {
        type: 'string',
        name: t('tradeConfig.instrument_id'),
        dataIndex: 'instrument_id',
        sorter: buildStrSorter('instrument_id'),
        width: 60,
      },
      {
        type: 'string',
        name: '',
        dataIndex: 'side',
        width: 80,
      },
      {
        type: 'string',
        name: '',
        dataIndex: 'offset',
        width: 40,
      },
      {
        type: 'number',
        name: t('tradeConfig.price'),
        dataIndex: 'price_resolved',
        width: 120,
        align: 'right',
        sorter: buildSorter('price'),
      },
      {
        type: 'number',
        name: t('tradeConfig.volume'),
        dataIndex: 'volume',
        width: 60,
        align: 'right',
        sorter: buildSorter('volume'),
      },
      {
        type: 'number',
        name: t('tradeConfig.latency_trade'),
        dataIndex: 'latency_trade',
        width: 90,
        align: 'right',
        sorter: buildTableColumnSorterWithStrike<KungfuApi.TradeResolved>(
          'num',
          'latency_trade',
        ),
      },
      {
        name:
          kfLocation.category == 'td'
            ? t('orderConfig.dest_uname')
            : t('orderConfig.source_uname'),
        dataIndex: kfLocation.category == 'td' ? 'dest_uname' : 'source_uname',
        sorter: buildStrSorter(
          kfLocation.category == 'td' ? 'dest_uname' : 'source_uname',
        ),
        flex: 1,
      },
      ...(isTdStrategyCategory(kfLocation.category)
        ? []
        : [
            {
              name: t('orderConfig.dest_uname'),
              dataIndex: 'dest_uname',
              sorter: buildStrSorter('dest_uname'),
              flex: 1,
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
    width: 40,
  },
  {
    name: '',
    dataIndex: 'offsetName',
    width: 40,
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

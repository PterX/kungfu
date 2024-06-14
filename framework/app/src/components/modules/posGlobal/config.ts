import { LedgerCategoryEnum } from '@kungfu-trader/kungfu-js-api/typings/enums';
import VueI18n from '@kungfu-trader/kungfu-js-api/language';
import { defaultColorMap } from '@kungfu-trader/kungfu-js-api/config/systemConfig';
import { useQuote } from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/actionsUtils';

import { DealTradingDataGetter } from '@kungfu-trader/kungfu-js-api/hooks/dealTradingDataHook';
import { getTradingDataSortKey } from '@kungfu-trader/kungfu-js-api/utils/busiUtils';
import {
  VTable,
  vTableSorter,
} from '@kungfu-trader/kungfu-app/src/renderer/assets/configs/vTable';
import { dealDirection } from '@kungfu-trader/kungfu-js-api/utils/tradingUtils';

const { t } = VueI18n.global;

const { getPositionLastPrice } = useQuote();
export { getPositionLastPrice };
export const getColumns = (): VTable.ColumnDefine[] => [
  {
    field: 'instrument_id',
    title: t('posGlobalConfig.instrument_id'),
    width: 156,
    sort: vTableSorter,
  },
  {
    field: 'direction',
    title: '',
    width: 44,
    style: {
      color: (args) => {
        return defaultColorMap[
          dealDirection(args.dataValue).color || 'default'
        ];
      },
    },
    fieldFormat: (args) => {
      return dealDirection(args.direction).name;
    },
  },
  {
    field: 'static_yesterday',
    title: t('posGlobalConfig.static_yesterday'),
    width: 74,
    style: {
      textAlign: 'right',
    },
    headerStyle: {
      textAlign: 'right',
    },
    sort: vTableSorter,
  },
  {
    field: 'open_volume',
    title: t('posGlobalConfig.open_volume'),
    width: 74,
    style: {
      textAlign: 'right',
    },
    headerStyle: {
      textAlign: 'right',
    },
    sort: vTableSorter,
  },
  {
    field: 'close_volume',
    title: t('posGlobalConfig.close_volume'),
    width: 74,
    style: {
      textAlign: 'right',
    },
    headerStyle: {
      textAlign: 'right',
    },
    sort: vTableSorter,
  },
  {
    field: 'yesterday_volume',
    title: t('posGlobalConfig.yesterday_volume'),
    width: 74,
    style: {
      textAlign: 'right',
    },
    headerStyle: {
      textAlign: 'right',
    },
    sort: vTableSorter,
  },
  {
    field: 'today_volume',
    title: t('posGlobalConfig.today_volume'),
    width: 74,
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
    title: t('posGlobalConfig.sum_volume'),
    width: 74,
    style: {
      textAlign: 'right',
    },
    headerStyle: {
      textAlign: 'right',
    },
    sort: vTableSorter,
  },
  {
    title: t('posGlobalConfig.avg_open_price'),
    field: 'avg_open_price_resolved',
    width: 98,
    style: {
      textAlign: 'right',
    },
    headerStyle: {
      textAlign: 'right',
    },
    sort: vTableSorter,
  },
  {
    field: 'last_price_resolved',
    title: t('posGlobalConfig.last_price'),
    width: 86,
    style: {
      textAlign: 'right',
    },
    headerStyle: {
      textAlign: 'right',
    },
    fieldFormat: (args) => {
      return getPositionLastPrice(args, 'last_price_resolved');
    },
    sort: vTableSorter,
  },
  {
    field: 'unrealized_pnl_resolved',
    title: t('posGlobalConfig.unrealized_pnl'),
    width: 98,
    style: {
      textAlign: 'right',
      color: (args) => {
        if (!Number(args.dataValue)) return defaultColorMap['text'];

        return +args.dataValue > 0
          ? defaultColorMap['red']
          : defaultColorMap['green'];
      },
    },
    headerStyle: {
      textAlign: 'right',
    },
    sort: vTableSorter,
  },
];

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

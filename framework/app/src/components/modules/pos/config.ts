import { DealTradingTableHooks } from '@kungfu-trader/kungfu-js-api/hooks/dealTradingTableHook';
import { isTd } from '@kungfu-trader/kungfu-js-api/utils/busiUtils';
import { sorter } from '@kungfu-trader/kungfu-js-api/utils/commonUtils';
import { defaultColorMap } from '@kungfu-trader/kungfu-js-api/config/systemConfig';

import { useQuote } from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/actionsUtils';
import { VTable } from '@kungfu-trader/kungfu-app/src/renderer/assets/configs/vTable';
import { dealDirection } from '@kungfu-trader/kungfu-js-api/utils/tradingUtils';
import VueI18n from '@kungfu-trader/kungfu-js-api/language';
const { t } = VueI18n.global;

const { getPositionLastPrice } = useQuote();
export { getPositionLastPrice };
export const getColumns = (
  kfLocation: KungfuApi.KfLocation,
): VTable.ColumnDefine[] =>
  (globalThis.HookKeeper.getHooks().dealTradingTable as DealTradingTableHooks)
    .trigger(kfLocation, 'position')
    .getColumns<VTable.ColumnDefine>([
      {
        field: 'instrument_id_resolved',
        title: t('posGlobalConfig.instrument_id'),
        sort: sorter,
        width: 190,
      },
      ...(isTd(kfLocation.category)
        ? []
        : [
            {
              field: 'account_id_resolved',
              title: t('posGlobalConfig.account_id_resolved'),
              width: 120,
              sort: sorter,
            },
          ]),
      {
        field: 'direction',
        title: '',
        width: 50,
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
        width: 110,
        style: {
          textAlign: 'right',
        },
        headerStyle: {
          textAlign: 'right',
        },
        sort: sorter,
      },
      {
        field: 'open_volume',
        title: t('posGlobalConfig.open_volume'),
        width: 110,
        style: {
          textAlign: 'right',
        },
        headerStyle: {
          textAlign: 'right',
        },
        sort: sorter,
      },
      {
        field: 'close_volume',
        title: t('posGlobalConfig.close_volume'),
        width: 110,
        style: {
          textAlign: 'right',
        },
        headerStyle: {
          textAlign: 'right',
        },
        sort: sorter,
      },
      {
        field: 'yesterday_volume',
        title: t('posGlobalConfig.yesterday_volume'),
        width: 110,
        style: {
          textAlign: 'right',
        },
        headerStyle: {
          textAlign: 'right',
        },
        sort: sorter,
      },
      {
        field: 'today_volume',
        title: t('posGlobalConfig.today_volume'),
        width: 110,
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
        title: t('posGlobalConfig.sum_volume'),
        width: 110,
        style: {
          textAlign: 'right',
        },
        headerStyle: {
          textAlign: 'right',
        },
        sort: sorter,
      },
      {
        field: 'frozen_total',
        title: t('posGlobalConfig.frozen_volume'),
        width: 110,
        style: {
          textAlign: 'right',
        },
        headerStyle: {
          textAlign: 'right',
        },
        sort: sorter,
      },
      {
        field: 'closable_volume',
        title: t('posGlobalConfig.closable_volume'),
        width: 110,
        style: {
          textAlign: 'right',
        },
        headerStyle: {
          textAlign: 'right',
        },
        sort: sorter,
      },

      {
        field: 'avg_open_price_resolved',
        title: t('posGlobalConfig.avg_open_price'),
        width: 110,
        style: {
          textAlign: 'right',
        },
        headerStyle: {
          textAlign: 'right',
        },
        sort: sorter,
      },
      {
        field: 'last_price_resolved',
        title: t('posGlobalConfig.last_price'),
        width: 110,
        style: {
          textAlign: 'right',
        },
        headerStyle: {
          textAlign: 'right',
        },
        fieldFormat: (args) => {
          return getPositionLastPrice(args, 'last_price_resolved') ?? '--';
        },
        sort: sorter,
      },
      {
        field: 'unrealized_pnl_resolved',
        title: t('posGlobalConfig.unrealized_pnl'),
        width: 110,
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
        sort: sorter,
      },
    ]);

import {
  InstrumentTypeEnum,
  OffsetEnum,
  SideEnum,
  PriceTypeEnum,
} from '@kungfu-trader/kungfu-js-api/typings/enums';
import { storeToRefs } from 'pinia';
import { dealOrderInputItem } from '@kungfu-trader/kungfu-js-api/utils/tradingUtils';
import { DEFAULT_PRECISION } from '@kungfu-trader/kungfu-js-api/utils/commonUtils';
import { useActiveInstruments } from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/actionsUtils';
import { useGlobalStore } from '@kungfu-trader/kungfu-app/src/renderer/pages/index/store/global';

import {
  getKfGlobalSettingsValue,
  setKfGlobalSettingsValue,
} from '@kungfu-trader/kungfu-js-api/config/globalSettings';
import globalBus from '@kungfu-trader/kungfu-js-api/utils/globalBus';
import { h, VNode } from 'vue';
import {
  makeOrderConfigKFTypes,
  makeOrderConfigKFKeys,
  orderInputTrans,
} from './config';
import VueI18n from '@kungfu-trader/kungfu-js-api/language';
import { getFutureArbitrageOrderTrans } from '../futureArbitrage/config';
import { buildCustomCheckboxVNode } from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/uiUtils';
const { t } = VueI18n.global;

const { globalSetting } = storeToRefs(useGlobalStore());

export function dealStockOffset(
  makeOrderInput: KungfuApi.MakeOrderInput,
): KungfuApi.MakeOrderInput {
  if (makeOrderInput.instrument_type == InstrumentTypeEnum.stock) {
    if (makeOrderInput.side == SideEnum.Buy) {
      makeOrderInput.offset = OffsetEnum.Open;
    }
    if (makeOrderInput.side == SideEnum.Sell) {
      makeOrderInput.offset = OffsetEnum.Close;
    }
  }
  return makeOrderInput;
}

export function dealOrderPlaceVNode(
  makeOrderInput: KungfuApi.MakeOrderInput,
  orderCount: number,
  isArbitrage: boolean,
): VNode {
  const orderData: KungfuApi.MakeOrderInput = dealStockOffset(makeOrderInput);
  const { getPriceTickAndPrecision } = useActiveInstruments();
  const priceType = makeOrderInput?.price_type;
  if (priceType === PriceTypeEnum.Market) {
    orderInputTrans['limit_price'] = t('tradingConfig.protect_price');
  }

  const currentOrderInputTrans = {
    ...orderInputTrans,
    ...(isArbitrage ? getFutureArbitrageOrderTrans(orderData.side) : {}),
  };

  const { price_precision } = getPriceTickAndPrecision(
    orderData.instrument_id,
    orderData.exchange_id,
  );
  const orderInputResolved: Record<string, KungfuApi.KfTradeValueCommonData> =
    dealOrderInputItem(orderData, price_precision ?? DEFAULT_PRECISION);

  return createOrderPlaceVNode(
    orderInputResolved,
    currentOrderInputTrans,
    orderCount,
  );
}

export const createOrderPlaceVNode = (
  orderInputResolved: Record<string, KungfuApi.KfTradeValueCommonData>,
  orderInputTrans: Record<string, string>,
  orderCount: number,
) => {
  const checkBoxVNode = buildCustomCheckboxVNode(
    !!globalSetting.value?.trade?.skipConfirmMakeOrder,
    t('tradingConfig.hide_next_time'),
    async (checked) => {
      if (checked) {
        const globalSetting = getKfGlobalSettingsValue();
        globalSetting.trade.skipConfirmMakeOrder = checked;

        try {
          await setKfGlobalSettingsValue(globalSetting);
          globalBus.next({
            tag: 'saved:globalSetting',
          });
          useGlobalStore().setKfGlobalSetting();
        } catch (error) {
          console.error('Failed to save global setting:', error);
        }
      }
    },
  );
  const vnode = Object.keys(orderInputResolved)
    .filter((key) => {
      if (orderInputResolved[key].name.toString() === '[object Object]') {
        return false;
      }
      return orderInputResolved[key].name !== '' && orderInputTrans[key];
    })
    .map((key) =>
      h('div', { class: 'trading-data-detail-row' }, [
        h('span', { class: 'label' }, `${orderInputTrans[key]}`),
        h(
          'span',
          {
            class: `value ${orderInputResolved[key].color}`,
            style: { color: `${orderInputResolved[key].color}` },
          },
          `${orderInputResolved[key].name}`,
        ),
      ]),
    );

  const rootBox = h('div', { class: 'root-node' }, [
    h('div', { class: 'trading-data-detail__warp' }, vnode),
    h('div', { class: 'ant-statistic apart-result-statistic order-number' }, [
      h(
        'div',
        { class: 'ant-statistic-title', style: 'font-size: 16px' },
        t('tradingConfig.make_order_number'),
      ),
      h(
        'div',
        { class: 'ant-statistic-content', style: 'font-size: 35px' },
        `${orderCount}`,
      ),
    ]),
    checkBoxVNode,
  ]);
  const rootVNode: VNode = h('div', { class: 'modal-node' }, rootBox);

  return rootVNode;
};

export const transformOrderInputToExtConfigForm = (
  orderInputFormState: Record<string, KungfuApi.KfConfigValue>,
  orderInputConfigSettings: KungfuApi.KfConfigItem[],
  extConfigSettings: KungfuApi.KfConfigItem[],
): Record<string, KungfuApi.KfConfigValue> => {
  const existedTypes = orderInputConfigSettings.map((item) => item.type);
  const existedKeys = orderInputConfigSettings.map((item) => item.key);
  return extConfigSettings.reduce((pre, configItem) => {
    const key = configItem.key;
    const type = configItem.type;

    if (type === 'td') {
      pre[key] = orderInputFormState['account_id'] || '';
      return pre;
    }

    const targetIndex = existedTypes.indexOf(type);
    if (
      targetIndex !== -1 &&
      (makeOrderConfigKFTypes.includes(type) ||
        makeOrderConfigKFKeys.includes(key))
    ) {
      const value = orderInputFormState[existedKeys[targetIndex]];
      pre[key] = value;
    }

    return pre;
  }, {} as Record<string, KungfuApi.KfConfigValue>);
};

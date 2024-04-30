import { dealUnderweightType } from '@kungfu-trader/kungfu-js-api/utils/busiUtils';
import { dealOrderInputItem } from '@kungfu-trader/kungfu-js-api/utils/tradingUtils';
import { VNode } from 'vue';
import { getBlockTradeOrderTrans } from './config';
import { createOrderPlaceVNode } from '../makeOrder/utils';
import {
  SideEnum,
  InstrumentTypeEnum,
  OffsetEnum,
  PriceTypeEnum,
} from '@kungfu-trader/kungfu-js-api/typings/enums';
import { useActiveInstruments } from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/actionsUtils';
import VueI18n from '@kungfu-trader/kungfu-js-api/language';
const { t } = VueI18n.global;

type OrderInputWithBlockMessage = KungfuApi.MakeOrderInput &
  KungfuApi.BlockMessage;

function dealStockOffset(
  makeOrderInput: OrderInputWithBlockMessage,
): OrderInputWithBlockMessage {
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

export const dealBlockMessageItem = (
  inputData: OrderInputWithBlockMessage,
): Record<string, KungfuApi.KfTradeValueCommonData> => {
  const orderInputResolved: Record<string, KungfuApi.KfTradeValueCommonData> =
    {};
  for (const key in inputData) {
    if (key === 'is_specific') {
      orderInputResolved[key] = dealUnderweightType(+inputData.is_specific);
    }
  }
  return orderInputResolved;
};

export function dealOrderPlaceVNode(
  makeOrderInput: KungfuApi.MakeOrderInput & KungfuApi.BlockMessage,
  orderCount: number,
): VNode {
  const orderData: OrderInputWithBlockMessage = dealStockOffset(makeOrderInput);
  const { getPriceTickAndPrecision } = useActiveInstruments();
  const priceType = makeOrderInput?.price_type;
  const transformMap: Record<string, string> = {};
  if (priceType === PriceTypeEnum.Market) {
    transformMap['limit_price'] = t('tradingConfig.protect_price');
  }

  const currentOrderInputTrans = getBlockTradeOrderTrans(transformMap);

  const blockMessageResolved: Record<string, KungfuApi.KfTradeValueCommonData> =
    dealBlockMessageItem(orderData);

  const { price_precision } = getPriceTickAndPrecision(
    orderData.instrument_id,
    orderData.exchange_id,
  );
  const orderInputResolved: Record<string, KungfuApi.KfTradeValueCommonData> =
    dealOrderInputItem(orderData, price_precision);

  return createOrderPlaceVNode(
    { ...orderInputResolved, ...blockMessageResolved },
    currentOrderInputTrans,
    orderCount,
  );
}

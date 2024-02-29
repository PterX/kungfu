import { OffsetEnum } from '@kungfu-trader/kungfu-js-api/typings/enums';
import { getOffsetByOffsetFilter } from '@kungfu-trader/kungfu-js-api/utils/busiUtils';
import {
  isShotable,
  isT0,
} from '@kungfu-trader/kungfu-js-api/utils/tradingUtils';

export const resolveTriggerOffset = (position: KungfuApi.PositionResolved) => {
  const { instrument_type, exchange_id } = position;
  if (isShotable(instrument_type) || isT0(instrument_type, exchange_id)) {
    if (exchange_id !== 'SHFE' && exchange_id !== 'INE')
      return OffsetEnum.Close;

    return position.yesterday_volume !== 0
      ? getOffsetByOffsetFilter('CloseYest', OffsetEnum.Close)
      : getOffsetByOffsetFilter('CloseToday', OffsetEnum.Close);
  } else {
    return OffsetEnum.Close;
  }
};

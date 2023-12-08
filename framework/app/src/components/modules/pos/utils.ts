import { OffsetEnum } from '@kungfu-trader/kungfu-js-api/typings/enums';
import { getOffsetByOffsetFilter } from '@kungfu-trader/kungfu-js-api/utils/busiUtils';
import { isShotable } from '@kungfu-trader/kungfu-js-api/utils/tradingUtils';

export const resolveTriggerOffset = (position: KungfuApi.PositionResolved) => {
  if (isShotable(position.instrument_type)) {
    return position.yesterday_volume !== BigInt(0)
      ? getOffsetByOffsetFilter('CloseYest', OffsetEnum.Close)
      : getOffsetByOffsetFilter('CloseToday', OffsetEnum.Close);
  } else {
    return OffsetEnum.Close;
  }
};

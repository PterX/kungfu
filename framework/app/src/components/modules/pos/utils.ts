import { OffsetEnum } from '@kungfu-trader/kungfu-js-api/typings/enums';
import {
  getOffsetByOffsetFilter,
  isShotable,
  isT0,
} from '@kungfu-trader/kungfu-js-api/utils/busiUtils';

export const resolveTriggerOffset = (position: KungfuApi.PositionResolved) => {
  if (isShotable(position.instrument_type)) {
    return position.yesterday_volume !== BigInt(0)
      ? getOffsetByOffsetFilter('CloseYest', OffsetEnum.Close)
      : getOffsetByOffsetFilter('CloseToday', OffsetEnum.Close);
  } else {
    return OffsetEnum.Close;
  }
};

export const getPosClosableVolumeByOffset = (
  position: KungfuApi.Position,
  offset: OffsetEnum,
) => {
  const isT0OrShotable =
    isT0(position.instrument_type, position.exchange_id) ||
    isShotable(position.instrument_type);
  const allVolume = position.volume - position.frozen_total,
    shotableYesterdayVolume =
      position.yesterday_volume - position.frozen_yesterday,
    yesterdayVolume = position.yesterday_volume - position.frozen_total,
    todayVolume = allVolume - yesterdayVolume;

  switch (offset) {
    case OffsetEnum.Close:
      return isT0OrShotable ? allVolume : yesterdayVolume;
    case OffsetEnum.CloseYest:
      return isT0OrShotable ? shotableYesterdayVolume : yesterdayVolume;
    case OffsetEnum.CloseToday:
      return isT0OrShotable ? todayVolume : 0n;
    default:
      return 0n;
  }
};

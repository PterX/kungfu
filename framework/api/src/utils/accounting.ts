import {
  hashInstrumentFactorUKey,
  hashInstrumentUKey,
} from '@kungfu-trader/kungfu-js-api/kungfu';
import {
  DirectionEnum,
  InstrumentTypeEnum,
} from '@kungfu-trader/kungfu-js-api/typings/enums';
import {
  dealKfDecimalPrecision,
  DEFAULT_PRECISION,
} from '@kungfu-trader/kungfu-js-api/utils/commonUtils';
import { getPrecisionByInstrumentType } from '@kungfu-trader/kungfu-js-api/utils/tradingUtils';

interface AccountingUsage {
  intrumentType: InstrumentTypeEnum;
  getTradeAmount: (
    watcher: KungfuApi.Watcher,
    instrumentForAccounting: KungfuApi.InstrumentForAccounting,
  ) => number | null;
  getInstrumentInWatcher: (
    watcher: KungfuApi.Watcher,
    instrumentId: string,
    exchangeId: string,
  ) => KungfuApi.Instrument | null;
  getMaxAvailableTradeVolume: (
    watcher: KungfuApi.Watcher,
    instrumentForAccounting: KungfuApi.InstrumentForAccounting,
    availAsset: number,
  ) => number | null;
}

export const AccountingInstrumentDefaultValue = {
  contract_multiplier: 10,
  long_margin_ratio: 0.1,
  short_margin_ratio: 0.1,
  exchange_rate: 1,
};

export const getInstrumentDefaultValue = (
  value,
  key: keyof typeof AccountingInstrumentDefaultValue,
  customDefaultValue?,
) => {
  return value || customDefaultValue || AccountingInstrumentDefaultValue[key];
};

abstract class BaseAccountingUsage implements AccountingUsage {
  intrumentType: InstrumentTypeEnum;
  constructor(intrumentType: InstrumentTypeEnum) {
    this.intrumentType = intrumentType;
  }

  abstract getTradeAmount(
    _watcher: KungfuApi.Watcher,
    _instrumentForAccounting: KungfuApi.InstrumentForAccounting,
  ): number | null;

  getInstrumentInWatcher(
    watcher: KungfuApi.Watcher,
    instrumentId: string,
    exchangeId: string,
  ) {
    const ukey = hashInstrumentUKey(instrumentId, exchangeId);
    return watcher.ledger.Instrument[ukey] as KungfuApi.Instrument | null;
  }

  getInstrumentFactorInWatcher(
    watcher: KungfuApi.Watcher,
    instrumentId: string,
    exchangeId: string,
    accountUID: number,
  ) {
    const ukey = hashInstrumentFactorUKey(instrumentId, exchangeId, accountUID);
    return watcher.ledger.InstrumentFactor[
      ukey
    ] as KungfuApi.InstrumentFactor | null;
  }

  getMaxAvailableTradeVolume(
    watcher: KungfuApi.Watcher,
    instrumentForAccounting: KungfuApi.InstrumentForAccounting,
    availAsset: number = 0,
  ) {
    const { instrumentId, exchangeId, price } = instrumentForAccounting;

    const instrument = this.getInstrumentInWatcher(
      watcher,
      instrumentId,
      exchangeId,
    );

    const precision = getPrecisionByInstrumentType(instrument?.instrument_type);

    return dealKfDecimalPrecision(availAsset / price, precision);
  }
}

function calcTradeAmountWithNoting(
  instrumentForAccounting: KungfuApi.InstrumentForAccounting,
  precision = DEFAULT_PRECISION,
) {
  const { price, volume } = instrumentForAccounting;
  return dealKfDecimalPrecision(price * volume, precision);
}

class DefaultAccountingUsage extends BaseAccountingUsage {
  constructor() {
    super(InstrumentTypeEnum.unknown);
  }

  getTradeAmount(
    watcher: KungfuApi.Watcher,
    instrumentForAccounting: KungfuApi.InstrumentForAccounting,
  ) {
    const { instrumentId, exchangeId } = instrumentForAccounting;

    const instrument = this.getInstrumentInWatcher(
      watcher,
      instrumentId,
      exchangeId,
    );

    const precision = getPrecisionByInstrumentType(instrument?.instrument_type);
    return calcTradeAmountWithNoting(instrumentForAccounting, precision);
  }
}

function calcTradeAmountForMain(
  instrumentForAccounting: KungfuApi.InstrumentForAccounting,
  instrumentFactor: KungfuApi.InstrumentFactor | null,
  precision = DEFAULT_PRECISION,
) {
  const { price, volume, direction } = instrumentForAccounting;
  const { exchange_rate, long_margin_ratio, short_margin_ratio } =
    instrumentFactor || {};
  const marginRatio =
    direction === DirectionEnum.Long
      ? getInstrumentDefaultValue(long_margin_ratio, 'long_margin_ratio', 1)
      : getInstrumentDefaultValue(short_margin_ratio, 'short_margin_ratio', 1);

  return dealKfDecimalPrecision(
    price *
      volume *
      marginRatio *
      getInstrumentDefaultValue(exchange_rate, 'exchange_rate'),
    precision,
  );
}

class StockAccountingUsage extends BaseAccountingUsage {
  constructor() {
    super(InstrumentTypeEnum.stock);
  }

  getTradeAmount(
    watcher: KungfuApi.Watcher,
    instrumentForAccounting: KungfuApi.InstrumentForAccounting,
  ) {
    if (!instrumentForAccounting) return null;

    const { instrumentId, exchangeId, accountUID } = instrumentForAccounting;

    const instrument = this.getInstrumentInWatcher(
      watcher,
      instrumentId,
      exchangeId,
    );

    const precision = getPrecisionByInstrumentType(instrument?.instrument_type);
    return calcTradeAmountForMain(
      instrumentForAccounting,
      this.getInstrumentFactorInWatcher(
        watcher,
        instrumentId,
        exchangeId,
        accountUID,
      ),
      precision,
    );
  }
}

class BondAccountingUsage extends BaseAccountingUsage {
  constructor() {
    super(InstrumentTypeEnum.bond);
  }

  getTradeAmount(
    watcher: KungfuApi.Watcher,
    instrumentForAccounting: KungfuApi.InstrumentForAccounting,
  ) {
    if (!instrumentForAccounting) return null;

    const { instrumentId, exchangeId, accountUID } = instrumentForAccounting;

    const instrument = this.getInstrumentInWatcher(
      watcher,
      instrumentId,
      exchangeId,
    );

    const precision = getPrecisionByInstrumentType(instrument?.instrument_type);
    return calcTradeAmountForMain(
      instrumentForAccounting,
      this.getInstrumentFactorInWatcher(
        watcher,
        instrumentId,
        exchangeId,
        accountUID,
      ),
      precision,
    );
  }
}

class FutureAccountingUsage extends BaseAccountingUsage {
  constructor() {
    super(InstrumentTypeEnum.future);
  }

  getTradeAmount(
    watcher: KungfuApi.Watcher,
    instrumentForAccounting: KungfuApi.InstrumentForAccounting,
  ) {
    if (!instrumentForAccounting) return null;

    const { price, volume, direction, instrumentId, exchangeId, accountUID } =
      instrumentForAccounting;

    const instrument = this.getInstrumentInWatcher(
      watcher,
      instrumentId,
      exchangeId,
    );

    const instrumentFactor = this.getInstrumentFactorInWatcher(
      watcher,
      instrumentId,
      exchangeId,
      accountUID,
    );

    const precision = getPrecisionByInstrumentType(instrument?.instrument_type);

    const { contract_multiplier } = instrument || {};

    const { long_margin_ratio, short_margin_ratio, exchange_rate } =
      instrumentFactor || {};

    if (direction === DirectionEnum.Long) {
      return dealKfDecimalPrecision(
        price *
          volume *
          getInstrumentDefaultValue(
            contract_multiplier,
            'contract_multiplier',
          ) *
          getInstrumentDefaultValue(long_margin_ratio, 'long_margin_ratio') *
          getInstrumentDefaultValue(exchange_rate, 'exchange_rate'),
        precision,
      );
    } else if (direction === DirectionEnum.Short) {
      return dealKfDecimalPrecision(
        price *
          volume *
          getInstrumentDefaultValue(
            contract_multiplier,
            'contract_multiplier',
          ) *
          getInstrumentDefaultValue(short_margin_ratio, 'short_margin_ratio') *
          getInstrumentDefaultValue(exchange_rate, 'exchange_rate'),
        precision,
      );
    }

    return null;
  }

  getMaxAvailableTradeVolume(
    watcher: KungfuApi.Watcher,
    instrumentForAccounting: KungfuApi.InstrumentForAccounting,
    availAsset: number = 0,
  ) {
    const { instrumentId, exchangeId, accountUID, price } =
      instrumentForAccounting;

    const instrument = this.getInstrumentInWatcher(
      watcher,
      instrumentId,
      exchangeId,
    );

    const instrumentFactor = this.getInstrumentFactorInWatcher(
      watcher,
      instrumentId,
      exchangeId,
      accountUID,
    );

    const { long_margin_ratio, short_margin_ratio, exchange_rate } =
      instrumentFactor || {};

    const { direction } = instrumentForAccounting;
    const precision = getPrecisionByInstrumentType(instrument?.instrument_type);
    const { contract_multiplier } = instrument || {};

    const marginRatio =
      (direction === DirectionEnum.Long
        ? getInstrumentDefaultValue(long_margin_ratio, 'long_margin_ratio')
        : getInstrumentDefaultValue(short_margin_ratio, 'short_margin_ratio')) *
      getInstrumentDefaultValue(exchange_rate, 'exchange_rate') *
      getInstrumentDefaultValue(contract_multiplier, 'contract_multiplier');

    return dealKfDecimalPrecision(
      availAsset / (price * marginRatio),
      precision,
    );
  }
}

class RepoAccountingUsage extends BaseAccountingUsage {
  constructor() {
    super(InstrumentTypeEnum.stock);
  }

  getTradeAmount(
    watcher: KungfuApi.Watcher,
    instrumentForAccounting: KungfuApi.InstrumentForAccounting,
  ) {
    const { volume, instrumentId, exchangeId, accountUID } =
      instrumentForAccounting;

    const instrument = this.getInstrumentInWatcher(
      watcher,
      instrumentId,
      exchangeId,
    );

    const precision = getPrecisionByInstrumentType(instrument?.instrument_type);

    const instrumentFactor = this.getInstrumentFactorInWatcher(
      watcher,
      instrumentId,
      exchangeId,
      accountUID,
    );

    const { exchange_rate } = instrumentFactor || {};
    return dealKfDecimalPrecision(
      volume * getInstrumentDefaultValue(exchange_rate, 'exchange_rate'),
      precision,
    );
  }

  getMaxAvailableTradeVolume(
    watcher: KungfuApi.Watcher,
    instrumentForAccounting: KungfuApi.InstrumentForAccounting,
    availAsset: number = 0,
  ) {
    const { instrumentId, exchangeId, accountUID, price } =
      instrumentForAccounting;

    const instrument = this.getInstrumentInWatcher(
      watcher,
      instrumentId,
      exchangeId,
    );

    const instrumentFactor = this.getInstrumentFactorInWatcher(
      watcher,
      instrumentId,
      exchangeId,
      accountUID,
    );

    const { exchange_rate } = instrumentFactor || {};

    const marginRatio = getInstrumentDefaultValue(
      exchange_rate,
      'exchange_rate',
    );

    const precision = getPrecisionByInstrumentType(instrument?.instrument_type);

    return dealKfDecimalPrecision(
      availAsset / (price * marginRatio),
      precision,
    );
  }
}

class CryptoAccountingUsage extends BaseAccountingUsage {
  constructor() {
    super(InstrumentTypeEnum.stock);
  }

  getTradeAmount(
    watcher: KungfuApi.Watcher,
    instrumentForAccounting: KungfuApi.InstrumentForAccounting,
  ) {
    const { instrumentId, exchangeId } = instrumentForAccounting;

    const instrument = this.getInstrumentInWatcher(
      watcher,
      instrumentId,
      exchangeId,
    );

    const precision = getPrecisionByInstrumentType(instrument?.instrument_type);
    return calcTradeAmountWithNoting(instrumentForAccounting, precision);
  }
}

const TradeAccountingUsageMap: Record<InstrumentTypeEnum, AccountingUsage> = {
  [InstrumentTypeEnum.unknown]: new DefaultAccountingUsage(),
  [InstrumentTypeEnum.stock]: new StockAccountingUsage(),
  [InstrumentTypeEnum.future]: new FutureAccountingUsage(),
  [InstrumentTypeEnum.fund]: new StockAccountingUsage(),
  [InstrumentTypeEnum.bond]: new BondAccountingUsage(),
  [InstrumentTypeEnum.stockoption]: new FutureAccountingUsage(),
  [InstrumentTypeEnum.techstock]: new StockAccountingUsage(),
  [InstrumentTypeEnum.index]: new StockAccountingUsage(),
  [InstrumentTypeEnum.repo]: new RepoAccountingUsage(),
  [InstrumentTypeEnum.crypto]: new CryptoAccountingUsage(),
  [InstrumentTypeEnum.cryptofuture]: new FutureAccountingUsage(),
  [InstrumentTypeEnum.cryptoufuture]: new FutureAccountingUsage(),
  [InstrumentTypeEnum.multi]: new DefaultAccountingUsage(),
};
export { TradeAccountingUsageMap };

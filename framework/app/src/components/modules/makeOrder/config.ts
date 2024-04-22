import {
  HedgeFlagEnum,
  InstrumentTypeEnum,
  OffsetEnum,
  PriceTypeEnum,
  SideEnum,
} from '@kungfu-trader/kungfu-js-api/typings/enums';
import {
  Side,
  MarginSideStatus,
} from '@kungfu-trader/kungfu-js-api/config/tradingConfig';
import VueI18n from '@kungfu-trader/kungfu-js-api/language';
import {
  getAbleHedgeFlag,
  enableCustomRadioType,
} from '@kungfu-trader/kungfu-js-api/utils/busiUtils';
import { isShotable } from '@kungfu-trader/kungfu-js-api/utils/tradingUtils';
const { t } = VueI18n.global;

export const LABEL_COL = 6;
export const WRAPPER_COL = 14;

export const getConfigSettings = ({
  location,
  instrumentType,
  isMarginMakeOrderSupport,
  isSpecifyContractSupport,
  side,
  priceType,
  pricePrecision,
  priceStep,
  sideList,
  offsetList,
  volumePrecision,
  volumeStep,
}: {
  location?:
    | KungfuApi.KfLocation
    | KungfuApi.KfLocationGroup
    | KungfuApi.KfConfig
    | null;
  instrumentType?: InstrumentTypeEnum;
  isMarginMakeOrderSupport?: boolean;
  isSpecifyContractSupport?: boolean;
  side?: SideEnum;
  priceType?: PriceTypeEnum;
  pricePrecision?: null | number;
  priceStep?: number;
  sideList?: string[];
  offsetList?: string[];
  volumePrecision?: number;
  volumeStep?: number;
}): KungfuApi.KfConfigItem[] => {
  const defaultSettings: KungfuApi.KfConfigItem[] = [
    location?.category === 'td'
      ? null
      : {
          key: 'account_id',
          name: t('tradingConfig.account'),
          type: 'td',
          required: true,
        },
    {
      key: 'instrument',
      name: t('tradingConfig.instrument'),
      type: 'instrument',
      required: true,
    },

    ...[
      isMarginMakeOrderSupport
        ? {
            key: 'side',
            name: t('tradingConfig.side'),
            type: 'marginSide',
            customRadioList: MarginSideStatus,
            default: SideEnum.GuaranteeStockBuy,
            required: true,
          }
        : {
            key: 'side',
            name: t('tradingConfig.side'),
            type: 'side',
            customRadioList: sideList || Object.keys(Side).slice(0, 2),
            default: SideEnum.Buy,
            required: true,
          },
    ],
    ...[
      isMarginMakeOrderSupport &&
      (side === SideEnum.RepayStock || side === SideEnum.RepayMargin)
        ? {
            key: 'contract_id',
            name: t('tradingConfig.specfy_contract'),
            type: 'contract',
            placeholder: t('tradingConfig.specfy_contract_placeholder'),
            disabled:
              side === SideEnum.RepayMargin ? !isSpecifyContractSupport : false,
          }
        : null,
    ],

    ...(isShotable(instrumentType || InstrumentTypeEnum.unknown) &&
    !isMarginMakeOrderSupport
      ? ([
          instrumentType === InstrumentTypeEnum.stockoption &&
          side === SideEnum.Exec
            ? null
            : {
                key: 'offset',
                name: t('tradingConfig.offset'),
                type: 'offset',
                customRadioList:
                  offsetList || Object.keys(enableCustomRadioType['offset']),
                default: OffsetEnum.Open,
                required: true,
              },
          instrumentType === InstrumentTypeEnum.future && getAbleHedgeFlag()
            ? {
                key: 'hedge_flag',
                name: t('tradingConfig.hedge'),
                type: 'hedgeFlag',
                default: HedgeFlagEnum.Speculation,
                required: true,
              }
            : null,
        ].filter((item) => !!item) as KungfuApi.KfConfigItem[])
      : []),
    {
      key: 'price_type',
      name: t('tradingConfig.price_type'),
      type: 'priceType',
      default: PriceTypeEnum.Limit,
      required: true,
    },
    {
      key: 'limit_price',
      name:
        priceType !== PriceTypeEnum.Market
          ? t('tradingConfig.price')
          : t('tradingConfig.protect_price'),
      type: 'float',
      min: 0,
      precision: pricePrecision ?? null,
      step: priceStep || 1,
      required: priceType !== PriceTypeEnum.Market ? true : false,
    },
    {
      key: 'volume',
      name: t('tradingConfig.volume'),
      type: volumePrecision ? 'float' : 'int',
      precision: volumePrecision ?? null,
      min: 0,
      step: volumeStep || 1,
      required: true,
    },
  ].filter((item) => !!item) as KungfuApi.KfConfigItem[];

  return defaultSettings;
};

export const makeOrderConfigKFTypes = [
  'td',
  'instrument',
  'side',
  'offset',
  'hedgeFlag',
  'priceType',
];

export const makeOrderConfigKFKeys = [
  'account_id',
  'instrument',
  'side',
  'offset',
  'volume',
];

export const orderInputTrans: Record<string, string> = {
  account_id: t('tradingConfig.account'),
  instrument_id: `${t('tradingConfig.instrument')}ID`,
  instrument_type: t('tradingConfig.instrument_type'),
  side: t('tradingConfig.side'),
  offset: t('tradingConfig.offset'),
  hedge_flag: t('tradingConfig.hedge'),
  price_type: t('tradingConfig.price_type'),
  volume: t('tradingConfig.volume'),
  exchange_id: `${t('globalSettingConfig.exchange_id')}ID`,
  limit_price: t('tradingConfig.limit_price'),
};

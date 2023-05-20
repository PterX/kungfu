import { Pm2ProcessStatusTypes } from '../utils/processUtils';

export enum BrokerStateStatusEnum {
  Pending = 0,
  Idle = 1,
  DisConnected = 2,
  Connected = 3,
  LoggedIn = 4,
  LoginFailed = 5,
  Ready = 100,
}

export type BrokerStateStatusTypes = keyof typeof BrokerStateStatusEnum;

export type ProcessStatusTypes =
  | Pm2ProcessStatusTypes
  | BrokerStateStatusTypes
  | StrategyStateStatusTypes;

export enum StrategyStateStatusEnum {
  Normal,
  Warn,
  Error,
}

export type StrategyStateStatusTypes = keyof typeof StrategyStateStatusEnum;

export enum LedgerCategoryEnum {
  td = 0,
  strategy = 1,
}

export type LedgerCategoryTypes = keyof typeof LedgerCategoryEnum;

export enum InstrumentTypeEnum {
  unknown,
  stock,
  stockoption,
  techstock,
  future,
  bond,
  fund,
  index,
  repo,
  crypto,
  cryptofuture,
  cryptoufuture,
  multi,
}
export type InstrumentTypes = keyof typeof InstrumentTypeEnum;

export enum BasketVolumeTypeEnum {
  Unknown,
  Quantity,
  Proportion,
}

export type BasketVolumeTypes = keyof typeof BasketVolumeTypeEnum;

export enum PriceLevelEnum {
  Latest, // 最新价
  Sell5,
  Sell4,
  Sell3,
  Sell2,
  Sell1,
  Buy1,
  Buy2,
  Buy3,
  Buy4,
  Buy5,
  UpperLimitPrice, // 涨停价
  LowerLimitPrice, // 跌停价
  Unknown,
}

export type PriceLevelTypes = keyof typeof PriceLevelEnum;

export type TdMdExtTypes = InstrumentTypes;

export type StrategyExtTypes = 'trade' | 'default' | 'unknown';

export type SystemExtTypes = 'service';

export type KfExtConfigTypes = TdMdExtTypes | StrategyExtTypes | SystemExtTypes;

export enum ExtRunForEnvTypesEnum {
  Ui = 'ui',
  Cli = 'cli',
}

export enum HedgeFlagEnum {
  Speculation,
  Arbitrage,
  Hedge,
  Covered,
}

export type HedgeFlagTypes = keyof typeof HedgeFlagEnum;

export enum PriceTypeEnum {
  Limit,
  Market,
  FakBest5,
  ForwardBest,
  ReverseBest,
  Fak,
  Fok,
  Unknown,
}

export type PriceTypes = keyof typeof PriceTypeEnum;

export enum VolumeConditionEnum {
  Any,
  Min,
  All,
}

export type VolumeConditionTypes = keyof typeof VolumeConditionEnum;

export enum TimeConditionEnum {
  IOC,
  GFD,
  GTC,
}

export type TimeConditionTypes = keyof typeof TimeConditionEnum;

export enum CommissionModeEnum {
  ByAmount,
  ByVolume,
}

export type CommissionModeTypes = keyof typeof CommissionModeEnum;

export enum OffsetEnum {
  Open,
  Close,
  CloseToday,
  CloseYest,
}

export type OffsetTypes = keyof typeof OffsetEnum;

export enum SideEnum {
  Buy,
  Sell,
  Lock,
  Unlock,
  Exec,
  Drop,
  Purchase,
  Redemption,
  Split,
  Merge,
  MarginTrade,
  ShortSell,
  RepayMargin,
  RepayStock,
  CashRepayMargin,
  StockRepayStock,
  SurplusStockTransfer,
  GuaranteeStockTransferIn,
  GuaranteeStockTransferOut,
  Unknown = 99,
}

export type SideTypes = keyof typeof SideEnum;

export enum DirectionEnum {
  Long,
  Short,
}

export enum UnderweightEnum {
  UnrestrictedShares,
  RestrictedShares,
}

export type DirectionTypes = keyof typeof DirectionEnum;

export enum OrderStatusEnum {
  Unknown,
  Submitted,
  Pending,
  Cancelled,
  Error,
  Filled,
  PartialFilledNotActive,
  PartialFilledActive,
  Lost,
}

export type OrderStatusTypes = keyof typeof OrderStatusEnum;

export enum BasketOrderStatusEnum {
  Unknown,
  Pending,
  PartialFilledNotActive, // 部分成交已结束
  PartialFilledActive, // 部分成交未结束
  Filled,
}

export enum KfCategoryEnum {
  md,
  td,
  strategy,
  system,
  operator,
}

export type KfCategoryTypes = keyof typeof KfCategoryEnum;

export type KfUIExtLocatorTypes =
  | 'sidebar'
  | 'sidebar_footer'
  | 'board'
  | 'global_setting'
  | 'make_order'
  | 'trading_task_view';

export type KfExhibitConfigTypes = 'form' | '';

export enum KfModeEnum {
  live,
  data,
  replay,
  backtest,
}

export type KfModeTypes = keyof typeof KfModeEnum;

export enum HistoryDateEnum {
  naturalDate,
  tradingDate,
}

export enum OrderActionFlagEnum {
  Cancel,
}

export enum FutureArbitrageCodeEnum {
  SP = 'SP',
  SPC = 'SPC',
  SPD = 'SPD',
  IPS = 'IPS',
}

export enum SpaceTabSettingEnum {
  SPACES = 'SPACES',
  TABS = 'TABS',
}

export enum SpaceSizeSettingEnum {
  FOURINDENT = 'FOURINDENT',
  TWOINDENT = 'TWOINDENT',
}

export enum OrderInputKeyEnum {
  VOLUME = 'VOLUME',
  PRICE = 'PRICE',
}

export enum SessionStatusEnum {
  Running,
  Finished,
}

export enum FrameMsgTypeEnum {
  Asset = 101,
  AssetMargin = 102,
  Position = 103,
  PositionEnd = 104,
  OrderInput = 201,
  Order = 202,
  Trade = 203,
  OrderAction = 204,
  OrderActionError = 205,
  BlockMessage = 206,
  OrderStat = 207,
  BasketOrder = 208,
  RequestHistoryOrder = 301,
  RequestHistoryTrade = 302,
  HistoryOrder = 303,
  HistoryTrade = 304,
  RequestHistoryOrderError = 305,
  RequestHistoryTradeError = 306,
  AssetRequest = 351,
  PositionRequest = 352,
  AssetSync = 353,
  PositionSync = 354,
  Quote = 401,
  Entrust = 402,
  Transaction = 403,
  Tree = 404,
  InstrumentKey = 501,
  CustomSubscribe = 502,
  SyntheticData = 601,
  frame_header = 10001,
  page_header = 10002,
  PageEnd = 10051,
  Time = 10052,
  Ping = 10053,
  Pong = 10054,
  Register = 10101,
  Deregister = 10102,
  Session = 10103,
  StrategyStateUpdate = 10104,
  OperatorStateUpdate = 10105,
  BrokerStateUpdate = 10106,
  SessionStart = 10151,
  SessionEnd = 10152,
  RequestStart = 10153,
  RequestStop = 10154,
  RequestDeregister = 10155,
  OperatorStateRequest = 10190,
  BrokerStateRequest = 10191,
  Config = 10201,
  RiskSetting = 10202,
  Commission = 10203,
  Instrument = 10204,
  Location = 10205,
  Basket = 10206,
  BasketInstrument = 10207,
  CacheReset = 10208,
  RequestCachedDone = 10209,
  CachedReadyToRead = 10251,
  RequestCached = 10252,
  RequestReadFrom = 10301,
  RequestReadFromPublic = 10302,
  RequestReadFromSync = 10303,
  RequestWriteTo = 10304,
  Channel = 10305,
  ChannelRequest = 10306,
  RequestWriteToBand = 10307,
  Band = 10308,
  RequestReadFromOthers = 10309,
  ResetBookRequest = 10401,
  MirrorPositionsRequest = 10402,
  KeepPositionsRequest = 10403,
  RebuildPositionsRequest = 10404,
  TimeRequest = 10501,
  TimeReset = 10502,
  TradingDay = 10503,
  TimeValue = 10601,
  TimeKeyValue = 10602,
}

export enum AddOperatorTypeEnum {
  File,
  Extension,
}

export enum CurrencyEnum {
  Unknown,
  CNY, // 人民币
  HKD, // 港币
  USD, // 美元
  JPY, // 日元
  GBP, // 英镑
  EUR, // 欧元
  CNH, // 离岸人民币
  SGD, // 新加坡元
  MYR, // 马来西亚吉特
  CEN, // 美分
}

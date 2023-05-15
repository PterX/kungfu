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
  PageEnd = 10000,
  SessionStart = 10001,
  SessionEnd = 10002,
  Time = 10003,
  Ping = 10008,
  Pong = 10009,
  RequestStop = 10024,
  RequestStart = 10025,
  CachedReadyToRead = 10060,
  RequestCached = 10061,
  ResetBookRequest = 400,
  MirrorPositionsRequest = 401,
  AssetRequest = 402,
  PositionRequest = 403,
  AssetSync = 404,
  PositionSync = 405,
  KeepPositionsRequest = 406,
  RebuildPositionsRequest = 407,
  InstrumentEnd = 802,
  Config = 10005,
  TimeValue = 20000,
  TimeKeyValue = 20001,
  StrategyStateUpdate = 20002,
  OperatorStateUpdate = 20003,
  Commission = 10006,
  RiskSetting = 10007,
  Session = 10010,
  Location = 10026,
  Register = 10011,
  Deregister = 10012,
  CacheReset = 10013,
  BrokerStateUpdate = 10014,
  RequestReadFrom = 10021,
  RequestReadFromPublic = 10022,
  RequestReadFromSync = 10031,
  RequestWriteTo = 10023,
  TradingDay = 10027,
  Channel = 10028,
  ChannelRequest = 10029,
  RequestCachedDone = 10062,
  TimeRequest = 10004,
  TimeReset = 10100,
  Instrument = 209,
  InstrumentKey = 210,
  CustomSubscribe = 303,
  Quote = 101,
  Entrust = 102,
  Transaction = 103,
  OrderInput = 201,
  BlockMessage = 207,
  OrderAction = 202,
  OrderActionError = 216,
  Order = 203,
  HistoryOrder = 212,
  Trade = 204,
  HistoryTrade = 213,
  Position = 205,
  PositionEnd = 800,
  Asset = 206,
  AssetMargin = 211,
  OrderStat = 215,
  SyntheticData = 301,
  RequestHistoryOrder = 10029,
  RequestHistoryTrade = 10030,
  RequestHistoryOrderError = 10031,
  RequestHistoryTradeError = 10032,
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

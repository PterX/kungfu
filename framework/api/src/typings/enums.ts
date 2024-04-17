import { Pm2ProcessStatusTypes } from '../typings/common';

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

export enum ContractTypeEnum {
  CrdBuyContract, //融资合约
  CrdSellContract, //融券合约
  CrdBuyInterest, //融资利息
  CrdSellFee, //融券费用
  CapitalRightsCompensation, //资金权益补偿
  ShareRightsCompensation, //股份权益补偿
  OverdueInterest, //逾期罚息
  BadDebtInterest, //坏账罚息
  Capital0ccupationFee, //资金占用费
  ManagementFee, //管理费
}

export type ContractTypes = keyof typeof ContractTypeEnum;

export enum CloseOutFlagEnum {
  NotCloseOut, //未了結
  Closeout, //了結
  InitNotc1o, //初始化未了結
}

export enum BasketVolumeTypeEnum {
  Unknown,
  Quantity,
  Proportion,
}

export type BasketVolumeTypes = keyof typeof BasketVolumeTypeEnum;

export enum BasketTypeEnum {
  Custom,
  ETF,
}

export enum ETFTypeEnum {
  LocalETF, // 本市ETF
  CrossCountryETF, // 跨境ETF
  CrossMarketETF, // 跨市ETF
  CurrencyETF, // 货币ETF
  PhysicalBondETF, // 实物债券ETF
  CommodityETF, // 商品ETF
  CashBondETF, // 现金债券ETF
  Unknown,
}

export enum ETFStatusEnum {
  Forbid, // 不允许申购也不允许赎回
  Allow, // 允许申购和赎回
  PurchaseOnly, // 只允许申购
  RedemptionOnly, // 只允许赎回
  Unknown,
}

// ETF成分股信息,标志改成分股是否可以由现金替代
export enum CashReplaceFlagEnum {
  UnReplace, // 不可替代
  EnReplace, // 可以替代
  MustReplace, // 必须替代
  UnSSEReplace, // 非沪市退补现金替代
  UnSSEMustReplace, // 非沪市必须现金替代
  UnSSESZEReplace, // 非沪深退补现金替代
  UnSSESZEMustReplace, // 非沪深必须现金替代
  UnHKReplace, // 港市退补现金替代
  UnHKMustReplace, // 港市必须现金替代
  Unknown,
}

export enum PriceLevelEnum {
  Latest, // 最新价
  Opposing5,
  Opposing4,
  Opposing3,
  Opposing2,
  Opposing1,
  Own1,
  Own2,
  Own3,
  Own4,
  Own5,
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
  EnhancedLimit,
  AtAuctionLimit,
  AtAuction,
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
  IOC, // 立即完成，否则撤销
  GFD, // 当日有效
  GTC, // 撤销前有效
  GFS, // 本节有效
  GTD, // 指定日期前有效
  GFA, // 集合竞价有效
  Unknown,
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
  Unknown = 99,
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
  GuaranteeStockBuy,
  GuaranteeStockSell,
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
  Cancelling,
  Pause,
  PendingSettlement,
}

export type OrderStatusTypes = keyof typeof OrderStatusEnum;

export enum KfExtTypeEnum {
  Unknown = 'unknown',
  Broker = 'broker',
  Task = 'task',
  Operator = 'operator',
  Service = 'service',
  UI = 'ui',
  Matcher = 'matcher',
  Indexer = 'indexer',
  Example = 'example',
  Factor = 'factor',
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
  | 'trading_task_view'
  | 'strategy_header_right'
  | 'extension_manager_use';

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
  Cancel, // 普通撤单
  TriggerCancel, // 预埋撤单
}

export enum AlgoOrderActionFlagEnum {
  Cancel,
  Start,
  Stop,
}

export enum OrderTriggerFlag {
  TriggerInsert, // 预埋下单
  TriggerCancel, // 预埋撤单
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

export enum OrderTriggerTypeEnum {
  Immediately, // 立即
  Touch, // 止损
  TouchProfit, // 止赢
  ParkedOrder, // 预埋单
  LastPriceGreaterThanStopPrice, // 最新价大于条件价
  LastPriceGreaterEqualStopPrice, // 最新价大于等于条件价
  LastPriceLesserThanStopPrice, // 最新价小于条件价
  LastPriceLesserEqualStopPrice, // 最新价小于等于条件价
  AskPriceGreaterThanStopPrice, // 卖一价大于条件价
  AskPriceGreaterEqualStopPrice, // 卖一价大于等于条件价
  AskPriceLesserThanStopPrice, // 卖一价小于条件价
  AskPriceLesserEqualStopPrice, // 卖一价小于等于条件价
  BidPriceGreaterThanStopPrice, // 买一价大于条件价
  BidPriceGreaterEqualStopPrice, // 买一价大于等于条件价
  BidPriceLesserThanStopPrice, // 买一价小于条件价
  BidPriceLesserEqualStopPrice, // 买一价小于等于条件价
}

// 预埋单类型
export enum OrderTriggerConfigTypeEnum {
  CancelOrder,
  MakeOrder,
}

export enum OrderTriggerStatusEnum {
  Unknown,
  Pending = OrderStatusEnum.Pending, // 等待中
  Submitted = OrderStatusEnum.Submitted, // 未触发
  Filled = OrderStatusEnum.Filled, // 已触发
  Cancelled = OrderStatusEnum.Cancelled, // 已取消
  Error = OrderStatusEnum.Error, // 错误
  Cancelling = OrderStatusEnum.Cancelling, // 待撤
}

export enum FundTransEnum {
  Pending,
  Success,
  Error,
}

export enum FundTransTypeEnum {
  BetweenNodes = 'between_nodes',
  TrancIn = 'tranc_in',
  TrancOut = 'tranc_out',
}

#include <cstdint>

namespace CICC {
namespace enums {

enum class InstrumentType : int8_t {
  Unknown,       // 未知
  Stock,         // 股票
  StockOption,   // 股票期权
  TechStock,     // 科技股
  Future,        // 期货
  Bond,          // 债券
  Fund,          // 基金
  Index,         // 指数
  Repo,          // 回购
  Crypto,        // 数字货币
  CryptoFuture,  // 数字货币期货
  CryptoUFuture, // 数字货币期货U本位
};

enum class OrderStatus : int8_t {
  Unknown,
  Submitted,              // 已提交
  Pending,                // 等待中
  Cancelled,              // 已撤单
  Error,                  // 错误
  Filled,                 // 已成交
  PartialFilledNotActive, // 部成部撤
  PartialFilledActive,    // 部成交易中
  Lost,                   // 丢失
  Cancelling,             // 待撤
  Pause,                  // 暂停
  PendingSettlement       // 等待结算
};

enum class Side : int8_t {
  Buy,                       // 买入
  Sell,                      // 卖出
  Lock,                      // 锁仓
  Unlock,                    // 解锁
  Exec,                      // 行权
  Drop,                      // 放弃行权
  Purchase,                  // 申购
  Redemption,                // 赎回
  Split,                     // 拆分
  Merge,                     // 合并
  MarginTrade,               // 融资买入
  ShortSell,                 // 融券卖出
  RepayMargin,               // 卖券还款
  RepayStock,                // 买券还券
  CashRepayMargin,           // 现金还款
  StockRepayStock,           // 现券还券
  SurplusStockTransfer,      // 余券划转
  GuaranteeStockTransferIn,  // 担保品转入
  GuaranteeStockTransferOut, // 担保品转出
  GuaranteeStockBuy,         // 担保品买入
  GuaranteeStockSell,        // 担保品卖出
  Unknown = 99
};

enum class Offset : int8_t { Open, Close, CloseToday, CloseYesterday };

enum class HedgeFlag : int8_t { Speculation, Arbitrage, Hedge, Covered };

enum class PriceType : int8_t {
  Limit, // 限价,证券通用
  Any, // 市价，证券通用，对于股票上海为最优五档剩余撤销，深圳为即时成交剩余撤销，建议客户采用
  FakBest5,       // 上海深圳最优五档即时成交剩余撤销，不需要报价
  ForwardBest,    // 深圳本方方最优价格申报, 不需要报价
  ReverseBest,    // 上海最优五档即时成交剩余转限价,
                  // 深圳对手方最优价格申报，不需要报价
  Fak,            // 深圳即时成交剩余撤销，不需要报价
  Fok,            // 深圳市价全额成交或者撤销，不需要报价
  EnhancedLimit,  // 增强限价盘-港股
  AtAuctionLimit, // 竞价限价盘-港股
  AtAuction,      // 竞价盘-港股| 期货(竞价盘的价格就是开市价格)
  Unknown
};

enum class VolumeCondition : int8_t { Any, Min, All };

enum class TimeCondition : int8_t { ///
  IOC,                              /// 立即完成，否则撤销
  GFD,                              /// 当日有效
  GTC,                              /// 撤销前有效
  GFS,                              /// 本节有效
  GTD,                              /// 指定日期前有效
  GFA,                              /// 集合竞价有效
  Unknown
};

enum class OrderActionFlag : int8_t {
  Cancel,        /// 普通撤单
  TriggerCancel, /// 预埋撤单
};

enum class AlgoOrderActionFlag : int8_t {
  Cancel, /// 普通撤单
  Start,  /// 启动
  Stop,   /// 停止
};

enum class PriceLevel : int8_t {
  Last, // 最新价
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
  Unknown
};

enum class Method : int8_t {
    round,
    direct,
};

enum class RoundReqType: int8_t{
    All,
    OrderInput,
    Order,
    Trade,
};

} // namespace enums
} // namespace CICC
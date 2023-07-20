// SPDX-License-Identifier: Apache-2.0

#ifndef WINGCHUN_REPORT_H
#define WINGCHUN_REPORT_H

#include <kungfu/longfist/longfist.h>
#include <kungfu/wingchun/book/bookkeeper.h>
#include <kungfu/yijinjing/practice/apprentice.h>
#include <unordered_map>

namespace kungfu::wingchun::tool {
class Report : public std::enable_shared_from_this<Report> {
public:
  Report() = default;
  virtual ~Report() = default;

  virtual std::string post_stop() { return {}; };

  // 行情数据更新回调
  //@param quote             行情数据
  virtual void on_quote(const longfist::types::Quote &quote, int64_t now){};

  // 逐笔委托更新回调
  //@param entrust           逐笔委托数据
  virtual void on_entrust(const longfist::types::Entrust &entrust, int64_t now){};

  // 逐笔成交更新回调
  //@param transaction       逐笔成交数据
  virtual void on_transaction(const longfist::types::Transaction &transaction, int64_t now){};

  // 行情数据更新回调
  // @param tree              行情数据
  virtual void on_tree(const longfist::types::Tree &tree, int64_t now){};

  // 接收合成数据更新回调
  // @param synthetic_data    合成数据
  virtual void on_read_synthetic_data(const longfist::types::SyntheticData &synthetic_data, int64_t now){};

  // 发出合成数据更新回调
  // @param synthetic_data    合成数据
  virtual void on_write_synthetic_data(const longfist::types::SyntheticData &synthetic_data, int64_t now){};

  // 订单信息更新回调
  // @param order             订单信息数据
  virtual void on_order(const longfist::types::Order &order, int64_t now){};

  // 订单成交回报回调
  // @param trade             订单成交数据
  virtual void on_trade(const longfist::types::Trade &trade, int64_t now){};
};
DECLARE_PTR(Report)

} // namespace kungfu::wingchun::tool

#endif // WINGCHUN_REPORT_H
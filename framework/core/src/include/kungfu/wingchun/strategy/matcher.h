// SPDX-License-Identifier: Apache-2.0

#ifndef WINGCHUN_MATCHER_H
#define WINGCHUN_MATCHER_H

#include <kungfu/longfist/longfist.h>
#include <kungfu/wingchun/book/bookkeeper.h>
// #include <kungfu/wingchun/strategy/strategy.h>
#include <kungfu/yijinjing/practice/apprentice.h>

namespace kungfu::wingchun::strategy {
FORWARD_DECLARE_CLASS_PTR(Runner)
class Matcher : public std::enable_shared_from_this<Matcher> {
public:
  Matcher() = default;
  virtual ~Matcher() = default;

  void set_runner(const Runner_ptr &runner) {
    runner_ = runner;
  }

  // 交易日切换回调
  //@param daytime 交易日
  virtual void on_trading_day(int64_t daytime){};

  // 行情数据更新回调
  //@param quote             行情数据
  virtual void on_quote(int64_t gen_time,
                        const longfist::types::Quote &quote){};

  // 逐笔委托更新回调
  //@param entrust           逐笔委托数据
  virtual void on_entrust(int64_t gen_time, const longfist::types::Entrust &entrust){};

  // 逐笔成交更新回调
  //@param transaction       逐笔成交数据
  virtual void on_transaction(int64_t gen_time, const longfist::types::Transaction &transaction){};

  // Operator publish 的 synthetic_data 回调
  //@param synthetic_data   Operator publish 的 synthetic_data
  virtual void on_synthetic_data(int64_t gen_time, const longfist::types::SyntheticData &synthetic_data){};

  // 订单信息更新回调
  //@param order             订单信息数据
  virtual void on_order_input(int64_t gen_time, const longfist::types::OrderInput &order_input){};

  // 订单操作回调
  //@param order             订单信息数据
  virtual void on_order_action(int64_t gen_time, const longfist::types::OrderAction &order_action){};        

  // 客户端状态变化回调
  //@param brokerStateUpdate     状态变化
  virtual void on_broker_state_change(int64_t gen_time, const longfist::types::BrokerStateUpdate &broker_state_update){};

  // 订阅的其他算子器状态变化回调
  //@param operator_state_update     状态变化
  virtual void on_operator_state_change(int64_t gen_time, const longfist::types::OperatorStateUpdate &operator_state_update){};                  

private:
  Runner_ptr runner_;
};
} // namespace kungfu::wingchun::strategy

#endif // WINGCHUN_MATCHER_H
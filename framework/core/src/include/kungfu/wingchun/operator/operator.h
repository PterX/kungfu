// SPDX-License-Identifier: Apache-2.0

//
// Created by Wei Jianan on 2022/12/10.
//

#ifndef WINGCHUN_OPERATOR_H
#define WINGCHUN_OPERATOR_H

#include <kungfu/longfist/longfist.h>
#include <kungfu/yijinjing/practice/apprentice.h>

namespace kungfu::wingchun::op {
FORWARD_DECLARE_CLASS_PTR(Context)

class Operator {
public:
  virtual ~Operator() = default;

  // 运行前
  virtual void pre_start(Context_ptr &context){};

  virtual void post_start(Context_ptr &context){};

  // 退出前
  virtual void pre_stop(Context_ptr &context){};

  virtual void post_stop(Context_ptr &context){};

  // 交易日切换回调
  //@param daytime 交易日
  virtual void on_trading_day(Context_ptr &context, int64_t daytime){};

  // 行情数据更新回调
  //@param quote             行情数据
  virtual void on_quote(Context_ptr &context, const longfist::types::Quote &quote,
                        const kungfu::yijinjing::data::location_ptr &location){};

  // 逐笔委托更新回调
  //@param entrust           逐笔委托数据
  virtual void on_entrust(Context_ptr &context, const longfist::types::Entrust &entrust,
                          const kungfu::yijinjing::data::location_ptr &location){};

  // 逐笔成交更新回调
  //@param transaction       逐笔成交数据
  virtual void on_transaction(Context_ptr &context, const longfist::types::Transaction &transaction,
                              const kungfu::yijinjing::data::location_ptr &location){};

  // TODO 考虑不提供python binding。
  // 逐笔成交更新回调
  //@param transaction       逐笔成交数据
  virtual void on_event(Context_ptr &context, const event_ptr &envet){};




  // 断开回调
  //@param deregister     断开数据
  virtual void on_deregister(Context_ptr &context, const longfist::types::Deregister &deregister,
                             const kungfu::yijinjing::data::location_ptr &location){};

  // 客户端状态变化回调
  //@param brokerStateUpdate     状态变化
  virtual void on_broker_state_change(Context_ptr &context,
                                      const longfist::types::BrokerStateUpdate &broker_state_update,
                                      const kungfu::yijinjing::data::location_ptr &location){};
  //   // 算子器状态变化回调
  // //@param brokerStateUpdate     状态变化
  // virtual void on_operator_state_change(Context_ptr &context,
  //                                     const longfist::types::BrokerStateUpdate &broker_state_update,
  //                                     const kungfu::yijinjing::data::location_ptr &location){};
};

DECLARE_PTR(Operator)
} // namespace kungfu::wingchun::op
#endif // WINGCHUN_OPERATOR_H

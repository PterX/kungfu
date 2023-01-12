// SPDX-License-Identifier: Apache-2.0
#ifndef WINGCHUN_OPERATOR_RUNNER_H
#define WINGCHUN_OPERATOR_RUNNER_H

#include <kungfu/wingchun/operator/operator.h>
#include <kungfu/wingchun/operator/runtime.h>
#include <kungfu/yijinjing/practice/apprentice.h>

namespace kungfu::wingchun::op {
class Runner : public yijinjing::practice::apprentice {
public:
  Runner(yijinjing::data::locator_ptr locator, const std::string &group, const std::string &name,
         longfist::enums::mode m, bool low_latency);

  ~Runner() override = default;

  [[nodiscard]] RuntimeContext_ptr get_context() const;

  void add_operator(const Operator_ptr &op);

  void on_exit() override;

  void on_trading_day(const event_ptr &event, int64_t daytime) override;

protected:
  void on_react() override;

  void on_start() override;

  void on_active() override;

  virtual RuntimeContext_ptr make_context();

  virtual void pre_start();

  virtual void post_start();

  virtual void pre_stop();

  virtual void post_stop();

private:
  //   bool positions_requested_ = false;
  bool broker_states_requested_ = false;
  //   bool positions_set_;
  bool started_;
  std::vector<Operator_ptr> operators_ = {};
  RuntimeContext_ptr context_;

  void prepare(const event_ptr &event);

  template <typename OnMethod = void (Operator::*)(Context_ptr &)> void invoke(OnMethod method) {
    auto context = std::dynamic_pointer_cast<Context>(context_);
    for (const auto &op : operators_) {
      (*op.*method)(context);
    }
  };

  template <typename TradingData, typename OnMethod = void (Operator::*)(Context_ptr &, const TradingData &)>
  void invoke(OnMethod method, const TradingData &data) {
    auto context = std::dynamic_pointer_cast<Context>(context_);
    for (const auto &op : operators_) {
      (*op.*method)(context, data);
    }
  };

  template <typename TradingData, typename OnMethod = void (Operator::*)(Context_ptr &, const TradingData &,
                                                                         const kungfu::yijinjing::data::location_ptr &)>
  void invoke(OnMethod method, const TradingData &data, const kungfu::yijinjing::data::location_ptr &location) {
    auto context = std::dynamic_pointer_cast<Context>(context_);
    for (const auto &op : operators_) {
      (*op.*method)(context, data, location);
    }
  };
};

static const int64_t NANO_MILLISECOND = int64_t(1000000);

} // namespace kungfu::wingchun::op

#endif // WINGCHUN_OPERATOR_RUNNER_H
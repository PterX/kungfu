#include <kungfu/wingchun/extension.h>
#include <kungfu/wingchun/operator/context.h>
#include <kungfu/wingchun/operator/operator.h>
#include <kungfu/yijinjing/journal/assemble.h>

using namespace kungfu::longfist::enums;
using namespace kungfu::longfist::types;
using namespace kungfu::wingchun::op;
using namespace kungfu::yijinjing::data;
int i = 0;
KUNGFU_MAIN_OPERATOR(KungfuOperator101) {
public:
  KungfuOperator101() = default;
  ~KungfuOperator101() = default;

  void pre_start(Context_ptr & context) override {
    SPDLOG_INFO("preparing operator");
    context->subscribe("sim", {"600000"}, {"SSE"});
  }

  void post_start(Context_ptr & context) override { SPDLOG_INFO("operator started"); }

  void on_quote(Context_ptr & context, const Quote &quote, const location_ptr &location, uint32_t dest) override {
    i++;
    std::string key = "price";
    std::string value = fmt::format("{}", quote.last_price);
    context->publish_synthetic_data(key, value);
    SPDLOG_INFO("on quote: {} i {} location->uid {}", quote.last_price, i, location->location_uid);
  }

  void on_broker_state_change(Context_ptr & context, const BrokerStateUpdate &broker_state_update,
                              const location_ptr &location) override {
    SPDLOG_WARN("on broker state changed: {}", broker_state_update.to_string());
  };

  void on_operator_state_change(Context_ptr & context, const OperatorStateUpdate &operator_state_update,
                                const location_ptr &location) override {
    SPDLOG_WARN("on operator state changed: {}", operator_state_update.to_string());
  };
};

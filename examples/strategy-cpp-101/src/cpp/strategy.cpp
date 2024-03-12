#include <kungfu/wingchun/extension.h>
#include <kungfu/wingchun/orderbook/depthorderbooks.h>
#include <kungfu/wingchun/orderbook/orderbooks.h>
#include <kungfu/wingchun/strategy/context.h>
#include <kungfu/wingchun/strategy/strategy.h>
#include <kungfu/yijinjing/journal/assemble.h>

using namespace kungfu::longfist::enums;
using namespace kungfu::longfist::types;
using namespace kungfu::wingchun::strategy;
using namespace kungfu::yijinjing::data;
using namespace kungfu::wingchun::orderbook;
int i = 0;
// auto orderbooks = DepthOrderbooks();
KUNGFU_MAIN_STRATEGY(KungfuStrategy101) {
public:
  KungfuStrategy101() = default;
  ~KungfuStrategy101() = default;

  void pre_start(Context_ptr & context) override {

    SPDLOG_INFO("preparing strategy");
    SPDLOG_INFO("arguments: {}", context->get_arguments());
    context->add_account("xtp", "15011218");
    context->subscribe("sim", {"600000"}, {"SSE"});
    context->subscribe("xtp", {"300059"}, {"SZE"});

    // context->attach_orderbooks(orderbooks);
  }

  void post_start(Context_ptr & context) override { SPDLOG_INFO("strategy started"); }

  void on_quote(Context_ptr & context, const Quote &quote, const location_ptr &location, uint32_t dest) override {
    SPDLOG_INFO("on quote: {} i {} location->uid {}", quote.last_price, i, location->location_uid);
  }

  void on_tree(Context_ptr & context, const Tree &tree, const location_ptr &location, uint32_t dest) override {
    SPDLOG_INFO("on tree: {}", tree.to_string());
  }

  void on_synthetic_data(Context_ptr & context, const SyntheticData &synthetic_data, const location_ptr &location,
                         uint32_t dest) override {
    SPDLOG_INFO("on_synthetic_data: {} ", synthetic_data.to_string());
  }

  void on_broker_state_change(Context_ptr & context, const BrokerStateUpdate &broker_state_update,
                              const location_ptr &location) override {
    SPDLOG_INFO("on broker state changed: {}", broker_state_update.to_string());
  };

  void on_operator_state_change(Context_ptr & context, const OperatorStateUpdate &operator_state_update,
                                const location_ptr &location) override {
    SPDLOG_INFO("on operator state changed: {}", operator_state_update.to_string());
  };

  void on_custom_data(Context_ptr & context, uint32_t msg_type, const std::vector<uint8_t> &data, uint32_t length,
                      const kungfu::yijinjing::data::location_ptr &location, uint32_t dest) override {
    SPDLOG_WARN("on_custom_data msg_type: {}", msg_type);
    SPDLOG_WARN("on_custom_data data: {}", reinterpret_cast<const char *>(data.data()));
    SPDLOG_WARN("on_custom_data length: {}", length);
  }

  void on_entrust(Context_ptr & context, const Entrust &entrust, const location_ptr &location, uint32_t dest) override {
    SPDLOG_INFO("--------------on_entrust-----------------------");
    // for (auto it = orderbooks.get_bids("300059", "SZE").begin(); it != orderbooks.get_bids("300059", "SZE").end();
    //      ++it) {
    //   SPDLOG_INFO("测试 Buy level: {}", (*it).to_string());
    // }

    // for (const auto &level : orderbooks.get_asks("300059", "SZE")) {
    //   SPDLOG_INFO("测试 Sell level: {}", level.to_string());
    // }
  }

  void on_transaction(Context_ptr & context, const Transaction &transaction, const location_ptr &location,
                      uint32_t dest) override {
    SPDLOG_INFO("--------------on_transaction-----------------------");
    // for (auto it = orderbooks.get_bids("300059", "SZE").begin(); it != orderbooks.get_bids("300059", "SZE").end();
    //      ++it) {
    //   SPDLOG_INFO("测试 Buy level: {}", (*it).to_string());
    // }

    // for (const auto &level : orderbooks.get_asks("300059", "SZE")) {
    //   SPDLOG_INFO("测试 Sell level: {}", level.to_string());
    // }
  };
};

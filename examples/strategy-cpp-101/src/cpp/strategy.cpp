#include <kungfu/wingchun/extension.h>
#include <kungfu/wingchun/strategy/context.h>
#include <kungfu/wingchun/strategy/runtime.h>
#include <kungfu/wingchun/strategy/strategy.h>
#include <kungfu/yijinjing/journal/assemble.h>

using namespace kungfu::longfist::enums;
using namespace kungfu::longfist::types;
using namespace kungfu::wingchun::strategy;
using namespace kungfu::yijinjing::data;
int i = 0;
KUNGFU_MAIN_STRATEGY(KungfuStrategy101) {
public:
  KungfuStrategy101() = default;
  ~KungfuStrategy101() = default;

  void pre_start(Context_ptr & context) override {
    SPDLOG_INFO("preparing strategy");
    SPDLOG_INFO("arguments: {}", context->get_arguments());
  }

  void post_start(Context_ptr & context) override {
    SPDLOG_INFO("strategy started");
    auto l_ptr = location::make_shared(mode::LIVE, category::MD, "sim", "sim", {});
    kungfu::yijinjing::journal::assemble asb(l_ptr, location::PUBLIC, AssembleMode::All);
    auto headers = asb.read_headers(Location{});
    for (const auto &head : headers) {
      SPDLOG_INFO("head: {}", head.to_string());
    }
    kungfu::yijinjing::journal::assemble asb2(l_ptr, location::PUBLIC, AssembleMode::All);
    auto locations = asb2.read_bytes<Location>();
    SPDLOG_INFO("locations.length: {}", locations.size());
    for (const auto &loc : locations) {
      SPDLOG_INFO("locaton byte: {}", std::string(loc.second.begin(), loc.second.end()));
    }
    kungfu::yijinjing::journal::assemble asb3(l_ptr, location::PUBLIC, AssembleMode::All);
    auto l3 = asb3.read_all<Location>();
    SPDLOG_INFO("locations.length: {}", l3.size());
    for (const auto &loc : l3) {
      SPDLOG_INFO("l3 : {}", loc.to_string());
    }
  }

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
};

#include <kungfu/wingchun/extension.h>
#include <kungfu/wingchun/orderbooks/depthorderbooks.h>
#include <kungfu/wingchun/orderbooks/orderbooks.h>
#include <kungfu/wingchun/strategy/context.h>
#include <kungfu/wingchun/strategy/strategy.h>
#include <kungfu/yijinjing/journal/assemble.h>

using namespace kungfu::longfist::enums;
using namespace kungfu::longfist::types;
using namespace kungfu::wingchun::strategy;
using namespace kungfu::yijinjing::data;
using namespace kungfu::wingchun::orderbook;
int i = 0;
KUNGFU_MAIN_STRATEGY(KungfuStrategy101) {
public:
  KungfuStrategy101() = default;
  ~KungfuStrategy101() = default;

  void pre_start(Context_ptr & context) override {
    SPDLOG_INFO("preparing strategy");
    SPDLOG_INFO("arguments: {}", context->get_arguments());
    context->add_account("xtp", "15011218");
    context->subscribe("sim", {"600000"}, {"SSE"});
    context->subscribe("xtp", {"600009"}, {"SSE"});
    SPDLOG_INFO("测试 start");
    context->set_orderbook(std::make_shared<DepthOrderbooks>());
    auto depth_orderbook = context->get_orderbook();
    // std::map<std::string, std::map<double, Level>> testMap;
    // testMap = {{"SZE:000001", {{1.1, Level(1.1, 100, 0)}, {2.2, Level(2.2, 200, 0)}}},
    //            {"SZE:000002", {{2.2, Level(2.2, 200, 0)}}}};
    // depth_orderbook->setBidMap(testMap);
    // auto bids = depth_orderbook->get_bids("000001", "SZE");
    // for (auto level : bids) {
    //   // SPDLOG_INFO("测试 base for 循环 level: {}", level.second.price);
    //   SPDLOG_INFO("测试 base for 循环 level.price: {}", level.price);
    //   SPDLOG_INFO("测试 base for 循环 level: {}", level.to_string());
    // }

    // for (auto it = bids.begin(); it != bids.end(); ++it) {
    //   SPDLOG_INFO("测试 迭代器 for 循环 level: {}", (*it).to_string());
    // }

    // SPDLOG_INFO("测试 end");
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
  
  void on_entrust()
};

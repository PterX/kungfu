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
    // context->add_account("sim", "1");
    context->subscribe("sim", {"600000"}, {"SSE"});
    context->subscribe_operator("operator-cpp-test1", "operator-cpp-test1");
  }

  void post_start(Context_ptr & context) override {
    SPDLOG_INFO("strategy started");
    // auto &runtime = dynamic_cast<RuntimeContext &>(*context);
    // auto &bookkeeper = runtime.get_bookkeeper();
    // auto &books = bookkeeper.get_books();
    // for (const auto &pair : books) {
    //   auto &book = pair.second;
    //   SPDLOG_INFO("book asset: {}", book->asset.to_string());
    // }
  }

  void on_quote(Context_ptr & context, const Quote &quote, const location_ptr &location) override {
    SPDLOG_INFO("on quote: {} i {} location->uid {}", quote.last_price, i, location->location_uid);
    // i++;
    // if (i == 5) {
    //   std::shared_ptr<kungfu::yijinjing::journal::assemble> p_assemble =
    //       std::make_shared<kungfu::yijinjing::journal::assemble>(std::vector<locator_ptr>{});
    //   std::shared_ptr<kungfu::yijinjing::journal::frame_reader> r = p_assemble->get_reader(location);
    //   auto f = r->current_frame();
    //   SPDLOG_INFO("f source {} dest {} data {}", f->source(), f->dest(), f->data_as_string());
    //   while (true) {
    //     auto f = r->next_frame();
    //     if (!f) {
    //       SPDLOG_INFO("f null");
    //       break;
    //     }
    //     SPDLOG_INFO("f source {} dest {} data {}", f->source(), f->dest(), f->data_as_string());
    //   }
    // }
  }

  void on_synthetic_data(Context_ptr & context, const SyntheticData &synthetic_data, const location_ptr &location) override {
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

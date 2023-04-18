// #include <kungfu/wingchun/extension.h>
#include <kungfu/wingchun/strategy/context.h>
#include <kungfu/wingchun/strategy/runner.h>
#include <kungfu/wingchun/strategy/runtime.h>
#include <kungfu/wingchun/strategy/strategy.h>
#include <kungfu/yijinjing/journal/assemble.h>

using namespace kungfu::longfist::enums;
using namespace kungfu::longfist::types;
using namespace kungfu::wingchun::strategy;
using namespace kungfu::yijinjing::data;
int i = 0;
// KUNGFU_MAIN_STRATEGY(KungfuStrategy101) {
class KungfuStrategy101 : public Strategy {
public:
  KungfuStrategy101() = default;
  ~KungfuStrategy101() = default;

  void pre_start(Context_ptr &context) override {
    SPDLOG_INFO("preparing strategy");
    SPDLOG_INFO("arguments: {}", context->arguments());
    context->add_account("sim", "123456");
    context->subscribe("sim", {"600000"}, {"SSE"});
  }

  void post_start(Context_ptr &context) override {
    SPDLOG_INFO("strategy started");
    auto &runtime = dynamic_cast<RuntimeContext &>(*context);
    auto &bookkeeper = runtime.get_bookkeeper();
    auto &books = bookkeeper.get_books();
    for (const auto &pair : books) {
      auto &book = pair.second;
      SPDLOG_INFO("book asset: {}", book->asset.to_string());
    }
  }

  void on_quote(Context_ptr &context, const Quote &quote, const location_ptr &location) override {
    SPDLOG_INFO("on quote: {}", quote.to_string());
    context->insert_order(quote.instrument_id, quote.exchange_id, "sim", "123456", quote.last_price, 200,
                          PriceType::Limit, Side::Buy, Offset::Open);
  }

  void on_broker_state_change(Context_ptr &context, const BrokerStateUpdate &broker_state_update,
                              const location_ptr &location) override {
    SPDLOG_INFO("on broker state changed: {}", broker_state_update.to_string());
  };
};

int main(int argc, char **argv) {
  SPDLOG_INFO("runner1 add strategy1");
  Runner runner(std::make_shared<locator>(), "CppStrategy", "demo01exe", mode::LIVE, false);
  SPDLOG_INFO("runner");
  runner.add_strategy(std::make_shared<KungfuStrategy101>());
  SPDLOG_INFO("add_strategy");
  SPDLOG_INFO("runner1 add strategy1");
  runner.run();
  return 0;
}
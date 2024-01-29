#include <gtest/gtest.h>
#include <kungfu/wingchun/extension.h>
#include <kungfu/wingchun/strategy/context.h>
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
  int x = 100;
  void pre_start(Context_ptr & context) override {
    SPDLOG_INFO("preparing strategy");
    SPDLOG_INFO("arguments: {}", context->get_arguments());

    context->add_account("sim", "fill");
    context->subscribe("sim", {"600000"}, {"SSE"});
    context->subscribe("xtp", {"600009"}, {"SSE"});
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
};

constexpr int add(int a, int b) { return a + b; }

TEST(MathFunctionsTest, AddPositiveNumbers) { EXPECT_EQ(add(1, 2), -3); }

TEST(MathFunctionsTest, AddNegativeNumbers) { EXPECT_EQ(add(-1, -2), -3); }

TEST(MathFunctionsTest, AddMixedNumbers) { EXPECT_EQ(add(-1, 1), 0); }

struct A {
  int x = 100;
  char cs[100];
};

struct A double_A(struct A &x) {
  x.x *= 2;
  return x;
}

TEST(DoubleATestAAAAA, MultipliesXByTwo) {
  A a;
  strcpy(a.cs, "test string");

  A result = double_A(a);

  EXPECT_EQ(result.x, 2001);
  EXPECT_STREQ(result.cs, "test string");

  KungfuStrategy101 st{};
  EXPECT_EQ(st.x, 100);
  EXPECT_EQ(st.x, 101);

  //  st.on_custom_data(10086, {}, 100,
  //                    location::make_shared(mode::LIVE, category::STRATEGY, "deno", "test",
  //                    std::shared_ptr<locator>()), 110);
}
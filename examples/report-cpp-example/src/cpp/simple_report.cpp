#include <kungfu/wingchun/common.h>
#include <kungfu/wingchun/extension.h>
#include <kungfu/wingchun/tool/report.h>
#include <spdlog/spdlog.h>

using namespace kungfu::longfist::enums;
using namespace kungfu::longfist::types;
using namespace kungfu::yijinjing::data;
using namespace kungfu::yijinjing::journal;
using namespace kungfu::wingchun::tool;
using kungfu::wingchun::get_direction;
using kungfu::yijinjing::time;

KUNGFU_MAIN_REPORT(SimpleReport) {
public:
  using Report::Report;

  void on_quote(const Quote &quote, int64_t now) override {
    // SPDLOG_INFO("on quote={}", quote.to_string());
  }

  void on_order(const Order &order, int64_t now) override { SPDLOG_INFO("on order={}", order.to_string()); }

  void on_trade(const Trade &trade, int64_t now) override {
    SPDLOG_INFO("on trade={}, at={}", trade.to_string(), time::strftime(now));
  }
};

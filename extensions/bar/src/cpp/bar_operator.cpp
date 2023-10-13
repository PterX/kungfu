#include <kungfu/wingchun/extension.h>
#include <kungfu/wingchun/operator/context.h>
#include <kungfu/wingchun/operator/operator.h>
#include <kungfu/wingchun/operator/runtime.h>

#include <iostream>

using namespace kungfu::longfist::enums;
using namespace kungfu::longfist::types;
using namespace kungfu::wingchun::op;
using namespace kungfu::yijinjing::data;

struct Bar {
  std::string instrument_id;
  std::string exchange_id;
  InstrumentType instrument_type;
  int64_t start_time;
  int64_t end_time;
  double open;
  double close;
  double high;
  double low;
  double volume;
  double start_volume;
  std::size_t tick_count;
};

void to_json(nlohmann::json &j, const Bar &bar) {
  j = nlohmann::json{{"instrument_id", bar.instrument_id},
                     {"exchange_id", bar.exchange_id},
                     {"instrument_type", bar.instrument_type},
                     {"start_time", bar.start_time},
                     {"end_time", bar.end_time},
                     {"open", bar.open},
                     {"close", bar.close},
                     {"high", bar.high},
                     {"low", bar.low},
                     {"volume", bar.volume}};
}

KUNGFU_MAIN_OPERATOR(BarOperator) {
public:
  BarOperator() = default;
  ~BarOperator() = default;

  std::pair<std::string, std::string> parse_exchange_instrument_id(const std::string &instrument_element) {
    std::string::size_type ex_pos_start = instrument_element.find_first_not_of('_', 0);
    std::string::size_type ex_pos_end = instrument_element.find_first_of('_', ex_pos_start);
    std::string::size_type ins_pos_start = instrument_element.find_first_of('_', 0) + 1;
    std::string::size_type ins_pos_end = instrument_element.find_first_of('_', ins_pos_start);
    return std::make_pair(instrument_element.substr(ex_pos_start, ex_pos_end - ex_pos_start),
                          instrument_element.substr(ins_pos_start, ins_pos_end - ins_pos_start));
  }

  void pre_start(Context_ptr & context) override {
    SPDLOG_INFO("preparing operator");
    try {
      nlohmann::json j_obj = nlohmann::json::parse(context->get_config());
      SPDLOG_INFO("preparing operator");
      std::string source = j_obj["source"];
      auto j_instruments = j_obj["instruments"];
      time_interval_ = j_obj["period"].get<int>() * kungfu::yijinjing::time_unit::NANOSECONDS_PER_SECOND;
      std::vector<std::string> instruments;
      for (auto it = j_instruments.begin(); it != j_instruments.end(); ++it) {
        auto [exchange, instrument_id] = parse_exchange_instrument_id(it.value());
        SPDLOG_INFO("exchange: {}, instrument_id: {}", exchange, instrument_id);
        context->subscribe(source, {instrument_id}, exchange);
      }
    } catch (std::exception &e) {
      SPDLOG_ERROR("failed to parse config: {}", e.what());
      exit(1);
    }
  }

  void post_start(Context_ptr & context) override { SPDLOG_INFO("operator started"); }

  void on_quote(Context_ptr & context, const Quote &quote, const location_ptr &location, uint32_t dest) override {
    SPDLOG_INFO("Quote: {}", quote.to_string());
    auto instrument_key = kungfu::wingchun::hash_instrument(quote.instrument_id, quote.exchange_id);
    auto pair = bars_.try_emplace(instrument_key);
    auto &bar = pair.first->second;
    if (pair.second) {
      bar.instrument_id = quote.instrument_id.to_string();
      bar.exchange_id = quote.exchange_id.to_string();
      bar.instrument_type = quote.instrument_type;
      auto current_time = context->now();
      bar.start_time = current_time - current_time % time_interval_;
      bar.end_time = bar.start_time + time_interval_;
    }
    if (context->now() >= bar.start_time && context->now() <= bar.end_time) {
      if (bar.tick_count == 0) {
        bar.high = quote.last_price;
        bar.low = quote.last_price;
        bar.open = quote.last_price;
        bar.close = quote.last_price;
        bar.start_volume = quote.volume;
      }
      bar.tick_count++;
      bar.volume = quote.volume - bar.start_volume;
      bar.high = std::max(bar.high, quote.last_price);
      bar.low = std::min(bar.low, quote.last_price);
      bar.close = quote.last_price;
    }
    if (context->now() >= bar.end_time) {
      context->publish_synthetic_data(fmt::format("{}_{}", bar.instrument_id, time_interval_),
                                      nlohmann::json(bar).dump());
      SPDLOG_INFO("publish_synthetic_data {} {}", fmt::format("{}_{}", bar.instrument_id, time_interval_),
                  nlohmann::json(bar).dump());
      bar.start_time = bar.end_time;
      while (bar.start_time + time_interval_ < context->now()) {
        bar.start_time += time_interval_;
      }
      bar.end_time = bar.start_time + time_interval_;
      if (bar.start_time <= context->now()) {
        bar.tick_count = 1;
        bar.start_volume = quote.volume;
        bar.volume = 0;
        bar.high = quote.last_price;
        bar.low = quote.last_price;
        bar.open = quote.last_price;
        bar.close = quote.last_price;
      } else {
        bar.tick_count = 0;
        bar.start_volume = 0;
        bar.volume = 0;
        bar.high = 0;
        bar.low = 0;
        bar.open = 0;
        bar.close = 0;
      }
    }
  }

  void on_broker_state_change(Context_ptr & context, const BrokerStateUpdate &broker_state_update,
                              const location_ptr &location) override {
    SPDLOG_INFO("on broker state changed: {}", broker_state_update.to_string());
  };

  void on_operator_state_change(Context_ptr & context, const OperatorStateUpdate &operator_state_update,
                                const location_ptr &location) override {
    SPDLOG_INFO("on operator state changed: {}", operator_state_update.to_string());
  };

private:
  int64_t time_interval_;
  std::unordered_map<uint32_t, Bar> bars_;
};

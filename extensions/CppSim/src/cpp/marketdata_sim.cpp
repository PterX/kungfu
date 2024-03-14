#include "marketdata_sim.h"
#include "common.h"

#include <ctime>
#include <random>

using namespace kungfu::yijinjing;
using namespace kungfu::yijinjing::data;
using namespace kungfu::longfist::types;
using namespace kungfu::longfist::enums;

namespace kungfu::wingchun::sim {

MarketDataSim::MarketDataSim(broker::BrokerVendor &vendor) : MarketData(vendor) {
  KUNGFU_SETUP_LOG();
  SPDLOG_DEBUG("arguments: {}", get_vendor().get_arguments());
}

MarketDataSim::~MarketDataSim() {
  transaction_band_writer_.reset();
  entrust_band_writer_.reset();
}

void MarketDataSim::pre_start() {
  SPDLOG_INFO("config: {}", get_config());
  config_ = nlohmann::json::parse(get_config());
  entrust_band_uid_ = request_band("market-data-band-entrust", 256);
  transaction_band_uid_ = request_band("market-data-band-transaction", 256);
}

void MarketDataSim::on_start() {
  update_broker_state(BrokerState::Ready);
  add_time_interval(int64_t(config_.quote * time_unit::NANOSECONDS_PER_SECOND),
                    [&](const auto &) { generate_quote(); });
  add_time_interval(int64_t(config_.tick * time_unit::NANOSECONDS_PER_SECOND), [&](const auto &) { generate_tick(); });
}

bool MarketDataSim::subscribe(const std::vector<InstrumentKey> &instrument_keys) {
  for (const auto &key : instrument_keys) {
    add_subscribe<Quote>(key, map_quote_);
    add_subscribe<Entrust>(key, map_entrust_);
    add_subscribe<Transaction>(key, map_transaction_);
    SPDLOG_INFO("InstrumentKey: {}", key.to_string());
  }
  return true;
}

bool MarketDataSim::subscribe_all() { return true; }

bool MarketDataSim::subscribe_custom(const longfist::types::CustomSubscribe &custom_sub) { return true; }

std::string MarketDataSim::make_exchange_instrument(const std::string &exchange_id, const std::string &instrument_id) {
  return fmt::format("{}:{}", exchange_id, instrument_id);
}

void MarketDataSim::on_band(const event_ptr &event) {
  const auto &band = event->data<Band>();
  if (band.dest_id == entrust_band_uid_) {
    entrust_band_writer_ = get_band_writer(entrust_band_uid_);
  } else if (band.dest_id == transaction_band_uid_) {
    transaction_band_writer_ = get_band_writer(transaction_band_uid_);
  }
}

void MarketDataSim::generate_quote() {
  // 创建随机数引擎，以当前时间作为种子
  static std::mt19937 rng(static_cast<unsigned int>(std::time(nullptr)));
  static std::uniform_int_distribution<int> dist_price(19000, 20000); // 随机last_price
  static std::uniform_int_distribution<int> dist_volume(1, 200);      // 随机volume
  static std::uniform_int_distribution<int> dist_delta(1, 50);        // 随机档位增量

  for (auto &pair : map_quote_) {
    auto &quote = pair.second;
    quote.last_price = dist_price(rng) / 100.0;
    quote.volume = dist_volume(rng) * get_min_volume(quote);
    double delta = dist_delta(rng) / 100.0;
    for (int i = 0; i < 10; ++i) {
      quote.bid_price[i] = quote.last_price - i * delta;
      quote.bid_volume[i] = dist_volume(rng) * get_min_volume(quote);
      quote.ask_price[i] = quote.last_price + i * delta;
      quote.ask_volume[i] = dist_volume(rng) * get_min_volume(quote);
    }
    SPDLOG_DEBUG("Quote: {}", quote.to_string());
    try_write_to(quote, 0);
  }
}

void MarketDataSim::generate_tick() {
  static std::mt19937 rng(static_cast<unsigned int>(std::time(nullptr)));
  static std::uniform_int_distribution<int> dist_price(19000, 20000); // 随机last_price
  static std::uniform_int_distribution<int> dist_volume(1, 200);      // 随机volume
  static std::uniform_int_distribution<int> dist_delta(1, 50);        // 随机档位增量
  if (entrust_band_writer_) {
    for (auto &pair : map_entrust_) {
      auto &entrust = pair.second;
      entrust.price = dist_price(rng) / 100.0;
      entrust.volume = dist_volume(rng) * get_min_volume(entrust);
      entrust.side = dist_delta(rng) >= 25 ? Side::Buy : Side::Sell;
      entrust.price_type = PriceType::Limit;
      entrust.data_time = time::now_in_nano();
      SPDLOG_DEBUG("Entrust: {}", entrust.to_string());
      entrust_band_writer_->write(now(), entrust);
    }
  }

  if (transaction_band_writer_) {
    for (auto &pair : map_transaction_) {
      auto &transaction = pair.second;
      transaction.price = dist_price(rng) / 100.0;
      transaction.volume = dist_volume(rng) * get_min_volume(transaction);
      transaction.side = dist_delta(rng) >= 25 ? Side::Buy : Side::Sell;
      transaction.exec_type = dist_delta(rng) >= 5 ? ExecType::Trade : ExecType::Cancel;
      transaction.data_time = time::now_in_nano();
      SPDLOG_DEBUG("Transaction: {}", transaction.to_string());
      transaction_band_writer_->write(now(), transaction);
    }
  }
}

} // namespace kungfu::wingchun::sim

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
  for (auto &pair : map_quote_) {
    auto &quote = pair.second;
    auto &bids_map_ = bid_orderbooks_[make_exchange_instrument(quote.exchange_id, quote.instrument_id)];
    auto &asks_map_ = ask_orderbooks_[make_exchange_instrument(quote.exchange_id, quote.instrument_id)];
    auto bid_iter = bids_map_.rbegin();
    auto ask_iter = asks_map_.begin();
    quote.last_price = map_last_price_[make_exchange_instrument(quote.exchange_id, quote.instrument_id)];
    quote.volume = map_volume_[make_exchange_instrument(quote.exchange_id, quote.instrument_id)];
    for (int i = 0; i < 10; ++i) {
      if (bid_iter != bids_map_.rend()) {
        quote.bid_price[i] = bid_iter->second.price;
        quote.bid_volume[i] = bid_iter->second.volume;
        ++bid_iter;
      } else {
        quote.bid_price[i] = 0;
        quote.bid_volume[i] = 0;
      }
      if (ask_iter != asks_map_.end()) {
        quote.ask_price[i] = ask_iter->second.price;
        quote.ask_volume[i] = ask_iter->second.volume;
        ++ask_iter;
      } else {
        quote.ask_price[i] = 0;
        quote.ask_volume[i] = 0;
      }
    }
    SPDLOG_DEBUG("Quote: {}", quote.to_string());
    try_write_to(quote, 0);
  }
}

void MarketDataSim::write_transaction_trade(const Entrust &entrust, double volume, int64_t order_no) {
  auto iter = map_transaction_.try_emplace(make_exchange_instrument(entrust.exchange_id, entrust.instrument_id));
  Transaction &transaction = iter.first->second;
  transaction.exchange_id = entrust.exchange_id;
  transaction.instrument_id = entrust.instrument_id;
  transaction.instrument_type = get_instrument_type(entrust.exchange_id, entrust.instrument_id);
  transaction.volume = volume;
  if (entrust.side == Side::Buy) {
    transaction.price =
        map_ask_entrust_[make_exchange_instrument(entrust.exchange_id, entrust.instrument_id)][order_no].price;
    transaction.side = Side::Sell;
    transaction.ask_no = order_no;
    transaction.bid_no = 0;
  } else {
    transaction.price =
        map_bid_entrust_[make_exchange_instrument(entrust.exchange_id, entrust.instrument_id)][order_no].price;
    transaction.side = Side::Buy;
    transaction.bid_no = order_no;
    transaction.ask_no = 0;
  }

  transaction.exec_type = ExecType::Trade;
  transaction.data_time = entrust.data_time;
  SPDLOG_DEBUG("Trade Transaction: {}", transaction.to_string());
  transaction_band_writer_->write(now(), transaction);
}

void MarketDataSim::generate_tick() {
  static std::mt19937 rng(static_cast<unsigned int>(std::time(nullptr)));
  static std::uniform_int_distribution<int> dist_price(19000,
                                                       20000);   // 随机last_price
  static std::uniform_int_distribution<int> dist_volume(1, 200); // 随机volume
  static std::uniform_int_distribution<int> dist_delta(1, 50);   // 随机档位增量

  if (transaction_band_writer_) {
    for (auto &pair : map_transaction_) {
      auto &transaction = pair.second;
      auto &bids_map_ = bid_orderbooks_[make_exchange_instrument(transaction.exchange_id, transaction.instrument_id)];
      auto &asks_map_ = ask_orderbooks_[make_exchange_instrument(transaction.exchange_id, transaction.instrument_id)];
      auto &bid_entrusts_ =
          map_bid_entrust_[make_exchange_instrument(transaction.exchange_id, transaction.instrument_id)];
      auto &ask_entrusts_ =
          map_ask_entrust_[make_exchange_instrument(transaction.exchange_id, transaction.instrument_id)];

      while (bids_map_.size() > 20 || asks_map_.size() > 20) {
        Side remove_side = bids_map_.size() > asks_map_.size() ? Side::Buy : Side::Sell;
        int64_t size = bids_map_.size() > asks_map_.size() ? bid_entrusts_.size() - 1 : ask_entrusts_.size() - 1;
        std::uniform_int_distribution<int> dist_entrust(0, size);
        auto it = bids_map_.size() > asks_map_.size() ? bid_entrusts_.begin() : ask_entrusts_.begin();
        std::advance(it, dist_entrust(rng));
        if (it->second.side != remove_side) {
          continue;
        }
        transaction.price = it->second.price;
        transaction.volume = it->second.volume;
        transaction.side = remove_side;
        transaction.exec_type = ExecType::Cancel;
        transaction.data_time = time::now_in_nano();
        if (remove_side == Side::Buy) {
          transaction.bid_no = it->first;
          transaction.ask_no = 0;
          auto &vec = bids_map_[it->second.price].order_no_vector;
          vec.erase(std::remove(vec.begin(), vec.end(), it->first), vec.end());
          bids_map_[it->second.price].volume -= it->second.volume;
          if (bids_map_[it->second.price].volume <= 0) {
            bids_map_.erase(it->second.price);
          }
          bid_entrusts_.erase(it);
        } else {
          transaction.ask_no = it->first;
          transaction.bid_no = 0;
          auto &vec = asks_map_[it->second.price].order_no_vector;
          vec.erase(std::remove(vec.begin(), vec.end(), it->first), vec.end());
          asks_map_[it->second.price].volume -= it->second.volume;
          if (asks_map_[it->second.price].volume <= 0) {
            asks_map_.erase(it->second.price);
          }
          ask_entrusts_.erase(it);
        }
        SPDLOG_DEBUG("Cancel Transaction: {}", transaction.to_string());
        transaction_band_writer_->write(now(), transaction);
      }
    }
  }

  if (entrust_band_writer_ and transaction_band_writer_) {
    for (auto &pair : map_entrust_) {
      auto &entrust = pair.second;
      entrust.price = dist_price(rng) / 100.0;
      entrust.volume = dist_volume(rng) * get_min_volume(entrust);
      entrust.side = dist_delta(rng) >= 25 ? Side::Buy : Side::Sell;
      entrust.price_type = PriceType::Limit;
      entrust.data_time = time::now_in_nano();
      entrust.orig_order_no =
          (entrust_band_writer_->current_frame_uid() ^ (entrust.data_time & 0xFFFFFFFF)) & 0x7FFFFFFFFFFFFFFF;
      SPDLOG_DEBUG("Entrust: {}", entrust.to_string());
      entrust_band_writer_->write(now(), entrust);
      if (entrust.side == Side::Buy) {
        map_bid_entrust_[make_exchange_instrument(entrust.exchange_id, entrust.instrument_id)][entrust.orig_order_no] =
            entrust;
      } else {
        map_ask_entrust_[make_exchange_instrument(entrust.exchange_id, entrust.instrument_id)][entrust.orig_order_no] =
            entrust;
      }
    }
    for (auto &pair : map_entrust_) {
      auto &entrust = pair.second;
      auto &bids_map_ = bid_orderbooks_[make_exchange_instrument(entrust.exchange_id, entrust.instrument_id)];
      auto &asks_map_ = ask_orderbooks_[make_exchange_instrument(entrust.exchange_id, entrust.instrument_id)];
      auto &bid_entrusts_ = map_bid_entrust_[make_exchange_instrument(entrust.exchange_id, entrust.instrument_id)];
      auto &ask_entrusts_ = map_ask_entrust_[make_exchange_instrument(entrust.exchange_id, entrust.instrument_id)];
      double volume = entrust.volume;
      if (entrust.side == Side::Buy) {
        if (bids_map_.find(entrust.price) != bids_map_.end()) {
          bids_map_[entrust.price].volume += volume;
          bids_map_[entrust.price].order_no_vector.emplace_back(entrust.orig_order_no);
          bids_map_[entrust.price].data_time = entrust.data_time;
        } else {
          while (!asks_map_.empty() && asks_map_.begin()->first <= entrust.price && volume > 0) {
            auto &vec = asks_map_.begin()->second.order_no_vector;
            std::vector<int64_t> to_remove;
            for (int64_t order_no : vec) {
              if (asks_map_.empty() || asks_map_.begin()->first > entrust.price || volume <= 0) {
                break;
              }
              if (ask_entrusts_[order_no].volume <= volume) {
                map_volume_[make_exchange_instrument(entrust.exchange_id, entrust.instrument_id)] =
                    ask_entrusts_[order_no].volume;
                map_last_price_[make_exchange_instrument(entrust.exchange_id, entrust.instrument_id)] =
                    ask_entrusts_[order_no].price;
                write_transaction_trade(entrust, ask_entrusts_[order_no].volume, order_no);
                to_remove.emplace_back(order_no);
                asks_map_.begin()->second.volume -= ask_entrusts_[order_no].volume;
                volume -= ask_entrusts_[order_no].volume;
                bid_entrusts_[entrust.orig_order_no].volume -= ask_entrusts_[order_no].volume;
                ask_entrusts_.erase(order_no);
              } else {
                map_volume_[make_exchange_instrument(entrust.exchange_id, entrust.instrument_id)] = volume;
                map_last_price_[make_exchange_instrument(entrust.exchange_id, entrust.instrument_id)] =
                    ask_entrusts_[order_no].price;
                write_transaction_trade(entrust, volume, order_no);
                ask_entrusts_[order_no].volume -= volume;
                bid_entrusts_.erase(entrust.orig_order_no);
                asks_map_.begin()->second.volume -= volume;
                volume = 0;
              }
            }
            for (int64_t order_no : to_remove) {
              vec.erase(std::remove(vec.begin(), vec.end(), order_no), vec.end());
            }
            if (vec.empty()) {
              asks_map_.erase(asks_map_.begin());
            }
          }
          if (volume != 0) {
            std::vector<int64_t> vector(1, entrust.orig_order_no);
            bids_map_[entrust.price] = Level(entrust.price, volume, entrust.data_time, vector);
          }
        }
      } else {
        if (asks_map_.find(entrust.price) != asks_map_.end()) {
          asks_map_.at(entrust.price).volume += volume;
          asks_map_[entrust.price].order_no_vector.emplace_back(entrust.orig_order_no);
          asks_map_.at(entrust.price).data_time = entrust.data_time;
        } else {
          while (!bids_map_.empty() && bids_map_.rbegin()->first >= entrust.price && volume > 0) {
            auto &vec = bids_map_.rbegin()->second.order_no_vector;
            std::vector<int64_t> to_remove;
            for (int64_t order_no : vec) {
              if (bids_map_.empty() || bids_map_.rbegin()->first < entrust.price || volume <= 0) {
                break;
              }
              if (bid_entrusts_[order_no].volume <= volume) {
                map_volume_[make_exchange_instrument(entrust.exchange_id, entrust.instrument_id)] =
                    bid_entrusts_[order_no].volume;
                map_last_price_[make_exchange_instrument(entrust.exchange_id, entrust.instrument_id)] =
                    bid_entrusts_[order_no].price;
                write_transaction_trade(entrust, bid_entrusts_[order_no].volume, order_no);
                to_remove.emplace_back(order_no);
                bids_map_.rbegin()->second.volume -= bid_entrusts_[order_no].volume;
                volume -= bid_entrusts_[order_no].volume;
                ask_entrusts_[entrust.orig_order_no].volume -= bid_entrusts_[order_no].volume;
                bid_entrusts_.erase(order_no);
              } else {
                map_volume_[make_exchange_instrument(entrust.exchange_id, entrust.instrument_id)] = volume;
                map_last_price_[make_exchange_instrument(entrust.exchange_id, entrust.instrument_id)] =
                    bid_entrusts_[order_no].price;
                write_transaction_trade(entrust, volume, order_no);
                bid_entrusts_[order_no].volume -= volume;
                ask_entrusts_.erase(entrust.orig_order_no);
                bids_map_.rbegin()->second.volume -= volume;
                volume = 0;
              }
            }
            for (int64_t order_no : to_remove) {
              vec.erase(std::remove(vec.begin(), vec.end(), order_no), vec.end());
            }
            if (vec.empty()) {
              bids_map_.erase(bids_map_.rbegin()->first);
            }
          }
          if (volume != 0) {
            std::vector<int64_t> vector(1, entrust.orig_order_no);
            asks_map_[entrust.price] = Level(entrust.price, volume, entrust.data_time, vector);
          }
        }
      }
    }
  }
}

} // namespace kungfu::wingchun::sim

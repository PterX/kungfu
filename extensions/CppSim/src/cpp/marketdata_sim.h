#ifndef KUNGFU_CPPSIM_EXT_MARKET_DATA_H
#define KUNGFU_CPPSIM_EXT_MARKET_DATA_H

#include <kungfu/wingchun/broker/marketdata.h>
#include <kungfu/yijinjing/common.h>

namespace kungfu::wingchun::sim {

struct Level final {
  double price;
  double volume;
  int64_t data_time;
  std::vector<int64_t> order_no_vector;

  Level() = default;

  Level(double p, double v, int64_t ut, const std::vector<int64_t> &vec = {})
      : price(p), volume(v), data_time(ut), order_no_vector(vec) {}
};

struct MDConfiguration {
  double quote;
  double tick;

  NLOHMANN_DEFINE_TYPE_INTRUSIVE(MDConfiguration, quote, tick)
};

class MarketDataSim : public broker::MarketData {
public:
  explicit MarketDataSim(broker::BrokerVendor &vendor);

  ~MarketDataSim() override;

  bool subscribe(const std::vector<longfist::types::InstrumentKey> &instrument_keys) override;

  bool subscribe_all() override;
  bool subscribe_custom(const longfist::types::CustomSubscribe &custom_sub) override;
  bool unsubscribe(const std::vector<longfist::types::InstrumentKey> &instrument_keys) override { return false; };
  void on_band(const event_ptr &event) override;

protected:
  void on_start() override;

  void pre_start() override;

private:
  MDConfiguration config_{};
  uint32_t entrust_band_uid_{};
  uint32_t transaction_band_uid_{};

  inline static thread_local yijinjing::journal::writer_ptr entrust_band_writer_{};
  inline static thread_local yijinjing::journal::writer_ptr transaction_band_writer_{};

  std::map<std::string, longfist::types::Entrust> map_entrust_{};
  std::map<std::string, longfist::types::Transaction> map_transaction_{};
  std::map<std::string, longfist::types::Quote> map_quote_{};
  std::map<std::string, std::map<int64_t, longfist::types::Entrust>>
      map_bid_entrust_{}; // 存放所有的买单逐笔委托 <委托号, Entrust>
  std::map<std::string, std::map<int64_t, longfist::types::Entrust>>
      map_ask_entrust_{};                          // 存放所有的卖单逐笔委托 <委托号, Entrust>
  std::map<std::string, double> map_last_price_{}; // 存放所有标的的最新价数据
  std::map<std::string, double> map_volume_{};     // 存放所有标的最后一笔撮合的volume数据

  std::map<std::string, std::map<double, Level>> bid_orderbooks_{}; // 存放所有标的的买单数据
  std::map<std::string, std::map<double, Level>> ask_orderbooks_{}; // 存放所有标的的买单数据
  static inline std::string make_exchange_instrument(const std::string &exchang_id, const std::string &instrument_id);

  void generate_quote();

  void generate_tick();

  void write_transaction_trade(const longfist::types::Entrust &entrust, double volume,
                               int64_t order_no); // 写成交类型的Transaction

  template <class T> void add_subscribe(const longfist::types::InstrumentKey &key, std::map<std::string, T> &map_) {
    auto pair = map_.try_emplace(make_exchange_instrument(key.exchange_id, key.instrument_id));
    if (pair.second) {
      T &data = pair.first->second;
      data.exchange_id = key.exchange_id;
      data.instrument_id = key.instrument_id;
      data.instrument_type = get_instrument_type(key.exchange_id, key.instrument_id);
    }
    bid_orderbooks_.try_emplace(make_exchange_instrument(key.exchange_id, key.instrument_id));
    ask_orderbooks_.try_emplace(make_exchange_instrument(key.exchange_id, key.instrument_id));
  }
};
} // namespace kungfu::wingchun::sim

#endif // KUNGFU_CPPSIM_EXT_MARKET_DATA_H

#ifndef KUNGFU_XTP_EXT_MARKET_DATA_H
#define KUNGFU_XTP_EXT_MARKET_DATA_H

#include <kungfu/wingchun/broker/marketdata.h>
#include <kungfu/yijinjing/common.h>

namespace kungfu::wingchun::sim {

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

  static inline std::string make_exchange_instrument(const std::string &exchang_id, const std::string &instrument_id);

  void generate_quote();

  void generate_tick();

  template <class T> void add_subscribe(const longfist::types::InstrumentKey &key, std::map<std::string, T> &map_) {
    auto pair = map_.try_emplace(make_exchange_instrument(key.exchange_id, key.instrument_id));
    if (pair.second) {
      T &data = pair.first->second;
      data.exchange_id = key.exchange_id;
      data.instrument_id = key.instrument_id;
      data.instrument_type = get_instrument_type(key.exchange_id, key.instrument_id);
    }
  }
};
} // namespace kungfu::wingchun::sim

#endif // KUNGFU_XTP_EXT_MARKET_DATA_H

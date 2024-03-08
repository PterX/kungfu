// SPDX-License-Identifier: Apache-2.0

#ifndef WINGCHUN_ORDERBOOK_H
#define WINGCHUN_ORDERBOOK_H

#include <kungfu/longfist/enums.h>
#include <kungfu/longfist/types.h>
#include <kungfu/wingchun/broker/client.h>
#include <kungfu/yijinjing/common.h>
#include <kungfu/yijinjing/practice/apprentice.h>

namespace kungfu::wingchun::orderbook {
struct Level final {
  double price;
  int64_t volume;
  int64_t data_time;

  Level() = default;

  Level(double p, int64_t v, int64_t ut) : price(p), volume(v), data_time(ut) {}

  std::string to_string() const {
    nlohmann::json j;
    j["price"] = price;
    j["volume"] = volume;
    j["data_time"] = data_time;
    return j.dump();
  }
};

class Orderbooks {
public:
  Orderbooks() = default;

  Orderbooks(const Orderbooks &) = delete;

  Orderbooks &operator=(const Orderbooks &) = delete;

  virtual ~Orderbooks() = default;

  void on_start(const rx::connectable_observable<event_ptr> &events); // todo 函数级别可能需要调整 可以利用友元函数处理
protected:
  virtual void on_entrust(const longfist::types::Entrust &entrust) = 0;
  virtual void on_transaction(const longfist::types::Transaction &transaction) = 0;
  virtual void on_quote(const longfist::types::Quote &quote) = 0;
};

template <typename OBS> class OrderbooksImpl : public Orderbooks {
public:

  const OBS &get_bids(std::string instrument_id, std::string exchange_id) {
    const auto instrument_exchange_id = instrument_id + exchange_id;
    if (bids_.find(instrument_exchange_id) == bids_.end()) {
      bids_.try_emplace(instrument_exchange_id, longfist::enums::Side::Buy);
    }
    return bids_.at(instrument_exchange_id);
  }

  const OBS &get_asks(std::string instrument_id, std::string exchange_id) {
    const auto instrument_exchange_id = instrument_id + exchange_id;
    if (asks_.find(instrument_exchange_id) == asks_.end()) {
      asks_.try_emplace(instrument_exchange_id, longfist::enums::Side::Sell);
    }
    return asks_.at(instrument_exchange_id);
  }

protected:
  void on_entrust(const longfist::types::Entrust &entrust) override {
    const_cast<OBS &>(get_bids(entrust.instrument_id, entrust.exchange_id)).on_entrust(entrust);
  }

  void on_transaction(const longfist::types::Transaction &transaction) override {
    const_cast<OBS &>(get_asks(transaction.instrument_id, transaction.exchange_id)).on_transaction(transaction);
  }
  
  void on_quote(const longfist::types::Quote &quote) override {
    const_cast<OBS &>(get_asks(quote.instrument_id, quote.exchange_id)).on_quote(quote);
  }

private:
  std::unordered_map<std::string, OBS> bids_;
  std::unordered_map<std::string, OBS> asks_;
};

class OrderbookSide {
public:
  OrderbookSide() = delete;

  OrderbookSide(const OrderbookSide &) = delete;

  OrderbookSide &operator=(const OrderbookSide &) = delete;

  virtual ~OrderbookSide() = default;

  longfist::enums::Side get_side() const { return side_; }
protected:
  OrderbookSide(longfist::enums::Side side) : side_(side){};

  void on_entrust(const longfist::types::Entrust &entrust){};

  void on_transaction(const longfist::types::Transaction &transaction){};

  void on_quote(const longfist::types::Quote &quote){};

private:
  longfist::enums::Side side_;
};


} // namespace kungfu::wingchun::orderbook
#endif // WINGCHUN_ORDERBOOK_H
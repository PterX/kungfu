// SPDX-License-Identifier: Apache-2.0

#ifndef WINGCHUN_ORDERBOOK_H
#define WINGCHUN_ORDERBOOK_H

#include <kungfu/longfist/types.h>
#include <kungfu/wingchun/book/accounting.h>
#include <kungfu/wingchun/book/staticdata.h>
#include <kungfu/wingchun/broker/client.h>
#include <kungfu/yijinjing/practice/apprentice.h>

using namespace kungfu::longfist::types;
namespace kungfu::wingchun::orderbook {
struct Level {
  double price;
  int64_t volume;
  int64_t data_time;

  Level() = default;

  Level(double p, int64_t v, int64_t ut) : price(p), volume(v), data_time(ut) {}

  virtual ~Level() = default;

  std::string to_string() const {
    nlohmann::json j;
    j["price"] = price;
    j["volume"] = volume;
    j["data_time"] = data_time;
    return j.dump();
  }
};

// 自定义迭代器
class LevelIterator {
public:
  LevelIterator() = default;

  virtual ~LevelIterator() = default;



  // virtual LevelIterator &operator++() = 0;

  // virtual LevelIterator operator++(int) = 0;

  // virtual const Level &operator*() const = 0;

  // virtual bool operator==(const LevelIterator &other) const = 0;

  // virtual bool operator!=(const LevelIterator &other) const = 0;
};

class OrderbookSide {
public:
  OrderbookSide() = default;

  virtual ~OrderbookSide() = default;

  virtual const LevelIterator &begin() const = 0;

  virtual const LevelIterator &end() const = 0;
};

class Orderbooks {
public:
  explicit Orderbooks() = default;

  virtual ~Orderbooks() = default;

  void on_start(const rx::connectable_observable<event_ptr> &events); // todo 函数级别可能需要调整 可以利用友元函数处理

  virtual OrderbookSide &get_bids(std::string instrument_id, std::string exchange_id) = 0;

  virtual OrderbookSide &get_asks(std::string instrument_id, std::string exchange_id) = 0;

protected:
  virtual void on_entrust(const event_ptr &event) = 0;

  virtual void on_transaction(const event_ptr &event) = 0;
};
} // namespace kungfu::wingchun::orderbook
#endif // WINGCHUN_ORDERBOOK_H
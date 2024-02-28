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
  int64_t update_time;
  int64_t level;
};

// 自定义迭代器
class LevelIterator {
public:
  // 构造函数
  LevelIterator(const std::map<int, Level>::const_iterator &iter) : current(iter) {}

  // 前缀递增运算符
  LevelIterator &operator++() {
    ++current;
    return *this;
  }

  // 后缀递增运算符
  LevelIterator operator++(int) {
    LevelIterator temp = *this;
    ++current;
    return temp;
  }

  // 解引用运算符
  const std::pair<const int, Level> &operator*() const { return *current; }

  // 相等运算符
  bool operator==(const LevelIterator &other) const { return current == other.current; }

  // 不等运算符
  bool operator!=(const LevelIterator &other) const { return current != other.current; }

private:
  std::map<int, Level>::const_iterator current;
};

class OrderbookSide {
public:
  // 默认构造函数
  OrderbookSide() = default;

  // 初始化列表构造函数
  OrderbookSide(const std::map<int, Level> &levels) : levels_(levels) {}

  // 返回 LevelIterator 的 begin
  LevelIterator begin() const { return LevelIterator(levels_.begin()); }

  // 返回 LevelIterator 的 end
  LevelIterator end() const { return LevelIterator(levels_.end()); }

private:
  std::map<int, Level> levels_; // 假设 levels_ 是 OrderbookSide 的成员变量
};

class Orderbook {
public:
  explicit Orderbook() {}

  virtual ~Orderbook() = default;

  void on_start(const rx::connectable_observable<event_ptr> &events);

  virtual OrderbookSide get_bids(std::string instrument_id, std::string exchange_id);
  // todo 返回类型可能要变 返回OrderbookSide类 这个类只有begin和end2个函数
  // begin和end分别返回LevelIterator对应的迭代器类型

  virtual OrderbookSide get_asks(std::string instrument_id, std::string exchange_id);

  virtual void on_entrust(const event_ptr &event); // 收到逐笔委托信息时维护该标的的orderbook

  virtual void on_transaction(const event_ptr &event); // 收到逐笔成交信息时维护该标的的orderbook

  // 设置 bid_map
  void setBidMap(const std::map<std::string, std::map<int, Level>> &bids) { bid_map = bids; }

  // 获取 bid_map
  const std::map<std::string, std::map<int, Level>> &getBidMap() const { return bid_map; }

  // 设置 ask_map
  void setAskMap(const std::map<std::string, std::map<int, Level>> &asks) { ask_map = asks; }

  // 获取 ask_map
  const std::map<std::string, std::map<int, Level>> &getAskMap() const { return ask_map; }

protected:
private:
  std::map<std::string, std::map<int, Level>> bid_map;
  std::map<std::string, std::map<int, Level>> ask_map;
};
} // namespace kungfu::wingchun::orderbook
#endif // WINGCHUN_ORDERBOOK_H
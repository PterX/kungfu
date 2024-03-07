#ifndef DEPTHORDERBOOK_H
#define DEPTHORDERBOOK_H

#include <kungfu/wingchun/orderbooks/orderbooks.h>

using namespace kungfu::longfist::enums;
using namespace kungfu::wingchun::orderbook;

namespace kungfu::wingchun::orderbook {

struct Compare {
  bool operator()() { return false; }
};

class DepthLevelIterator : public LevelIterator {
public:
  DepthLevelIterator() : current(), end() {}

  DepthLevelIterator(const std::map<double, Level, Compare>::const_iterator &current,
                     const std::map<double, Level, Compare>::const_iterator &end)
      : current(current), end(end) {}

  virtual ~DepthLevelIterator() = default;

  LevelIterator &operator++() {
    ++current;
    return *this;
  }

  LevelIterator operator++(int) {
    DepthLevelIterator temp = *this;
    ++current;
    return temp;
  }

  const Level &operator*() const { return (*current).second; }

  bool operator==(const DepthLevelIterator &other) const { return current == other.current; }

  bool operator!=(const DepthLevelIterator &other) const { return current != other.iter(); }

  std::map<double, Level, Compare>::const_iterator getEnd() const { return end; }

  std::map<double, Level, Compare>::const_iterator iter() const { return current; }

private:
  std::map<double, Level, Compare>::const_iterator current;
  std::map<double, Level, Compare>::const_iterator end; // 需要设置一个结束迭代的标志
};

class DepthOrderbookSide : public OrderbookSide {
public:
  DepthOrderbookSide() = default;
  // 暂时不用引用 因为利用这个构造函数创建迭代器实例是在get_bids、get_asks里操作的 在函数里创建的迭代器实例直接传回来
  // 如果传引用的话 在函数结束时会销毁
  DepthOrderbookSide(const DepthLevelIterator &begin, const DepthLevelIterator &end, Side side)
      : begin_(begin), end_(end), side_(side) {}

  const LevelIterator &begin() const override { return begin_; }

  const LevelIterator &end() const override { return end_; }

  // on_entrust(const event_ptr &event);// todo 计算放在这里 通过on_entrust 把begin和end值设置好 bid_map
  // ask_map是用来获取自定义迭代器的

private:
  // std::string exchange_id_;
  // std::string instrument_id_;
  // std::map<double, Level, Compare> &bid_map;
  // std::map<double, Level, Compare> &ask_map;
  DepthLevelIterator begin_ = {};
  DepthLevelIterator end_ = {};
  Side side_ = {};
};

class DepthOrderbooks : public Orderbooks {
public:
  explicit DepthOrderbooks() = default;

  virtual ~DepthOrderbooks() = default;

  OrderbookSide &get_bids(std::string instrument_id, std::string exchange_id) override;

  OrderbookSide &get_asks(std::string instrument_id, std::string exchange_id) override;

  // 覆盖基类的虚函数
  void on_entrust(const event_ptr &event) override;

  void on_transaction(const event_ptr &event) override;

private:
  std::map<std::string, DepthOrderbookSide> bid_sides_;
  std::map<std::string, DepthOrderbookSide> ask_sides_;
};
} // namespace kungfu::wingchun::orderbook

#endif // DEPTHORDERBOOK_H

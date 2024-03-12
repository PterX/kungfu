#ifndef WINGCHUN_QUOTE_ORDERBOOK_H
#define WINGCHUN_QUOTE_ORDERBOOK_H

#include <kungfu/wingchun/orderbook/orderbooks.h>
#include <kungfu/yijinjing/time.h>

using namespace kungfu::longfist::enums;
using namespace kungfu::longfist::types;
using namespace kungfu::yijinjing;
namespace kungfu::wingchun::orderbook {

class DepthOrderbook;
class BidirectionMapOrderbookSide : public OrderbookSide {
  using Container = std::map<double, Level>;

public:
  class iterator { // implements ForwardIterator
  public:
    typedef Level value_type;
    typedef const value_type *pointer;
    typedef const value_type &reference;
    typedef ptrdiff_t difference_type;
    typedef std::forward_iterator_tag iterator_category;

    explicit iterator(Container::const_iterator iter, Container::const_reverse_iterator reiter, Side side)
        : iter_(iter), reiter_(reiter), side_(side) {}

    reference operator*() const { return is_bid() ? reiter_->second : iter_->second; }
    pointer operator->() const { return is_bid() ? &(reiter_->second) : &(iter_->second); }
    iterator &operator++() {
      if (is_bid())
        ++reiter_;
      else
        ++iter_;
      return *this;
    }
    iterator operator++(int) {
      iterator temp = *this;
      if (is_bid())
        ++reiter_;
      else
        ++iter_;
      return temp;
    }

    bool operator==(const iterator &rhs) const { return is_bid() ? reiter_ == rhs.reiter_ : iter_ == rhs.iter_; }
    bool operator!=(const iterator &rhs) const { return !operator==(rhs); }

  private:
    bool is_bid() const { return side_ == Side::Buy; }
    Container::const_iterator iter_;
    Container::const_reverse_iterator reiter_;
    Side side_;
  };

  iterator begin() const { return iterator(levels_.begin(), levels_.rbegin(), get_side()); }

  iterator end() const { return iterator(levels_.end(), levels_.rend(), get_side()); }

  BidirectionMapOrderbookSide(longfist::enums::Side side) : OrderbookSide(side) {}

private:
  friend DepthOrderbook;
  Container levels_;
  std::unordered_map<int, Level> map_seq_id_2_level_;
};
class DepthOrderbook : public Orderbook<BidirectionMapOrderbookSide, BidirectionMapOrderbookSide> {
public:
  void on_entrust(const longfist::types::Entrust &entrust);
  void on_transaction(const longfist::types::Transaction &transaction);
  bool is_new_day(int64_t data_time);
};

using DepthOrderbooks = OrderbooksImpl<DepthOrderbook>;

} // namespace kungfu::wingchun::orderbook

#endif // WINGCHUN_QUOTE_ORDERBOOK_H

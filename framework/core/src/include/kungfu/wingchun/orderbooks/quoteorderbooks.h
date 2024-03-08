#ifndef QUOTEORDERBOOK_H
#define QUOTEORDERBOOK_H

#include <kungfu/wingchun/orderbooks/orderbooks.h>

using namespace kungfu::longfist::enums;
using namespace kungfu::longfist::types;

namespace kungfu::wingchun::orderbook {

class QuoteOrderbookSide : public OrderbookSide {
  using Container = std::array<Level, 10>;

public:
  class iterator { // implements ForwardIterator
  public:
    typedef Level value_type;
    typedef const value_type *pointer;
    typedef const value_type &reference;
    typedef ptrdiff_t difference_type;
    typedef std::forward_iterator_tag iterator_category;

    explicit iterator(Container::const_iterator iter) { iter_ = iter; }

    reference operator*() const { return *iter_; }
    pointer operator->() const { return &*iter_; }
    iterator &operator++() {
      ++iter_;
      return *this;
    }
    iterator operator++(int) {
      iterator temp = *this;
      ++iter_;
      return temp;
    }

    bool operator==(const iterator &rhs) const { return iter_ == rhs.iter_; }
    bool operator!=(const iterator &rhs) const { return !operator==(rhs); }

  private:
    Container::const_iterator iter_;
  };
  iterator begin() const { return iterator(levels_.begin()); }

  iterator end() const { return iterator(levels_.end()); }

  using OrderbookSide::OrderbookSide;
  void on_quote(const Quote &quote) override;

private:
  Container levels_;
};

using QuoteOrderbooks = OrderbooksImpl<QuoteOrderbookSide>;

} // namespace kungfu::wingchun::orderbook

#endif // QUOTEORDERBOOK_H

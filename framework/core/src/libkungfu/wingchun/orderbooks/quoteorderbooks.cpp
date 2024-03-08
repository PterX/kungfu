#include <kungfu/wingchun/orderbooks/quoteorderbooks.h>

namespace kungfu::wingchun::orderbook {

  void QuoteOrderbookSide::on_quote(const Quote &quote) {
    if (get_side() == Side::Buy) {
      for (int i = 0; i < 10; i++) {
        levels_[i].price = quote.bid_price[i];
        levels_[i].volume = quote.bid_volume[i];
      }
    } else {
      for (int i = 0; i < 10; i++) {
        levels_[i].price = quote.ask_price[i];
        levels_[i].volume = quote.ask_volume[i];
      }
    }  
  }
} // namespace kungfu::wingchun::orderbook

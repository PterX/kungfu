// SPDX-License-Identifier: Apache-2.0

#include <kungfu/wingchun/orderbook/orderbook.h>

using namespace kungfu::rx;
using namespace kungfu::longfist::enums;
using namespace kungfu::longfist::types;
using namespace kungfu::wingchun::broker;
using namespace kungfu::wingchun::map;
using namespace kungfu::yijinjing::practice;
using namespace kungfu::yijinjing;
using namespace kungfu::yijinjing::data;
using namespace kungfu::yijinjing::util;

namespace kungfu::wingchun::orderbook {

void Orderbook::on_start(const rx::connectable_observable<event_ptr> &events) {
  events | is(Entrust::tag) | $$(on_entrust(event));
  events | is(Transaction::tag) | $$(on_transaction(event));
}

void Orderbook::on_entrust(const event_ptr &event) {}

void Orderbook::on_transaction(const event_ptr &event) {}

OrderbookSide Orderbook::get_bids(std::string instrument_id, std::string exchange_id) { return OrderbookSide(); }

OrderbookSide Orderbook::get_asks(std::string instrument_id, std::string exchange_id) { return OrderbookSide(); }

} // namespace kungfu::wingchun::orderbook
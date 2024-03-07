// SPDX-License-Identifier: Apache-2.0

#include <kungfu/wingchun/orderbooks/orderbooks.h>

using namespace kungfu::rx;
using namespace kungfu::longfist::types;

namespace kungfu::wingchun::orderbook {

void Orderbooks::on_start(const rx::connectable_observable<event_ptr> &events) {
  events | is(Entrust::tag) | $$(on_entrust(event));
  events | is(Transaction::tag) | $$(on_transaction(event));
}

} // namespace kungfu::wingchun::orderbook
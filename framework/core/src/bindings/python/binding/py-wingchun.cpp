// SPDX-License-Identifier: Apache-2.0

#include "py-wingchun.h"

namespace kungfu::wingchun {
void bind(pybind11::module &&m) {
  pybind::bind_map_types(m);
  pybind::bind_book(m);
  pybind::bind_orderbook(m);
  pybind::bind_stream_data_batcher(m);
  pybind::bind_factor(m);
  pybind::bind_broker(m);
  pybind::bind_service(m);
  pybind::bind_strategy(m);
  pybind::bind_utils(m);
  pybind::bind_operator(m);
  pybind::bind_tool(m);
}
} // namespace kungfu::wingchun

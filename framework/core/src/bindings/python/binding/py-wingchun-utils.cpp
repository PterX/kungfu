// SPDX-License-Identifier: Apache-2.0

#include "py-wingchun.h"

#include <kungfu/longfist/types.h>
#include <kungfu/wingchun/common.h>

using namespace kungfu::longfist::types;
using namespace kungfu::wingchun;

namespace kungfu::wingchun::pybind {
void bind_utils(pybind11::module &m) {
  auto m_utils = m.def_submodule("utils");
  m_utils.def("hash_product", pybind11::overload_cast<const char *, const char *>(&hash_product));
  m_utils.def("hash_instrument", pybind11::overload_cast<const char *, const char *>(&hash_instrument));
  m_utils.def("hash_instrument", pybind11::overload_cast<uint32_t, const char *, const char *>(&hash_instrument));
  m_utils.def("is_valid_price", &is_valid_price);
  m_utils.def("is_final_status", &is_final_status);
  m_utils.def("get_instrument_type", &get_instrument_type);
  m_utils.def("order_from_input", [](const OrderInput &input) {
    Order order = {};
    order_from_input(input, order);
    return order;
  });
  m_utils.def("trade_from_order", [](const Order &order) {
    Trade trade{};
    trade_from_order(order, trade);
    return trade;
  });
  m_utils.def("order_trigger_from_input", [](const OrderTriggerInput &input) {
    OrderTrigger trigger{};
    order_trigger_from_input(input, trigger);
    return trigger;
  });
  m_utils.def("order_trigger_from_order", [](const Order &order) {
    OrderTrigger trigger{};
    order_trigger_from_order(order, trigger);
    return trigger;
  });
  m_utils.def("order_input_from_trigger_order", [](const OrderTriggerInput &trigger_input) {
    OrderInput order_input{};
    order_input_from_trigger_order(trigger_input, order_input);
    return order_input;
  });
}
} // namespace kungfu::wingchun::pybind

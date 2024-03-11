// SPDX-License-Identifier: Apache-2.0

#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>

#include <kungfu/wingchun/orderbook/depthorderbooks.h>
#include <kungfu/wingchun/orderbook/orderbooks.h>

using namespace kungfu::longfist;
using namespace kungfu::longfist::types;
using namespace kungfu::longfist::enums;
using namespace kungfu::yijinjing;
using namespace kungfu::yijinjing::data;
using namespace kungfu::yijinjing::journal;
using namespace kungfu::wingchun;
using namespace kungfu::wingchun::orderbook;

namespace py = pybind11;
namespace kungfu::wingchun::pybind {

void bind_orderbook(pybind11::module &m) {

  py::class_<Level, std::shared_ptr<Level>>(m, "Level")
      .def_readonly("price", &Level::price)
      .def_readonly("volume", &Level::volume)
      .def_readonly("data_time", &Level::data_time)
      .def("__repr__", &Level::to_string);

  py::class_<OrderbookSide, std::shared_ptr<OrderbookSide>>(m, "OrderbookSide")
      .def_property_readonly("side", &OrderbookSide::get_side);

  py::class_<BidirectionMapOrderbookSide, std::shared_ptr<BidirectionMapOrderbookSide>, OrderbookSide>(
      m, "BidirectionMapOrderbookSide")
      .def(
          "__iter__",
          [](const BidirectionMapOrderbookSide &orderbook_side) {
            return py::make_iterator(orderbook_side.begin(), orderbook_side.end());
          },
          py::keep_alive<0, 1>());

  py::class_<Orderbooks, std::shared_ptr<Orderbooks>>(m, "Orderbooks");

  py::class_<DepthOrderbooks, std::shared_ptr<DepthOrderbooks>, Orderbooks>(m, "DepthOrderbooks")
      .def(py::init<>())
      .def("get_bids", &DepthOrderbooks::get_bids, py::return_value_policy::reference)
      .def("get_asks", &DepthOrderbooks::get_asks, py::return_value_policy::reference);
}
} // namespace kungfu::wingchun::pybind
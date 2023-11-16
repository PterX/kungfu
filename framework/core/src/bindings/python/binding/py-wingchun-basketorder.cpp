#include "py-wingchun.h"
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>

#include <kungfu/wingchun/basketorder/basketorder.h>
#include <kungfu/wingchun/basketorder/basketorderengine.h>

using namespace kungfu::longfist;
using namespace kungfu::longfist::types;
using namespace kungfu::yijinjing;
using namespace kungfu::yijinjing::data;
using namespace kungfu::yijinjing::journal;
using namespace kungfu::wingchun;
using namespace kungfu::wingchun::basketorder;

namespace py = pybind11;

namespace kungfu::wingchun::pybind {

void bind_basketorder(pybind11::module &m) {
  py::class_<BasketOrderEngine, std::shared_ptr<BasketOrderEngine>>(m, "BasketOrderEngine")
      .def_property_readonly("baskets", &BasketOrderEngine::get_baskets)
      .def_property_readonly("basket_instruments", &BasketOrderEngine::get_basket_instruments);
}
} // namespace kungfu::wingchun::pybind
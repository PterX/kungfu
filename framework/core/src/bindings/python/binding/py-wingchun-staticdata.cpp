#include "py-wingchun.h"
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>

#include <kungfu/wingchun/staticdata/staticdata.h>

using namespace kungfu::longfist;
using namespace kungfu::longfist::types;
using namespace kungfu::yijinjing;
using namespace kungfu::yijinjing::data;
using namespace kungfu::yijinjing::journal;
using namespace kungfu::wingchun;
using namespace kungfu::wingchun::staticdata;

namespace py = pybind11;

namespace kungfu::wingchun::pybind {

void bind_staticdata(pybind11::module &m) {
  py::class_<StaticData, std::shared_ptr<StaticData>>(m, "StaticData")
      .def_property_readonly("baskets", &StaticData::get_baskets)
      .def_property_readonly("basket_instruments", &StaticData::get_basket_instruments)
      .def_property_readonly("commissions", &StaticData::get_commissions)
      .def_property_readonly("instruments", &StaticData::get_instruments)
      .def_property_readonly("instrument_factors", &StaticData::get_instrument_factors);
}
} // namespace kungfu::wingchun::pybind
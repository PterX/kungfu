// SPDX-License-Identifier: Apache-2.0

#include "py-wingchun.h"

#include <kungfu/wingchun/service/ledger.h>

using namespace kungfu::longfist::enums;
using namespace kungfu::longfist::types;
using namespace kungfu::yijinjing::data;
using namespace kungfu::yijinjing::journal;
using namespace kungfu::yijinjing::practice;
using namespace kungfu::wingchun::service;

namespace py = pybind11;

namespace kungfu::wingchun::pybind {
void bind_service(pybind11::module &m) {
  py::class_<Ledger, apprentice, std::shared_ptr<Ledger>>(m, "Ledger")
      .def(py::init<locator_ptr, mode, bool, const std::string &>())
      .def_property_readonly("io_device", &Ledger::get_io_device)
      .def_property_readonly("usable", &Ledger::is_usable)
      .def_property_readonly("bookkeeper", &Ledger::get_bookkeeper, py::return_value_policy::reference)
      .def("set_begin_time", &Ledger::set_begin_time)
      .def("set_end_time", &Ledger::set_end_time)
      .def("get_begin_time", &Ledger::get_begin_time)
      .def("get_end_time", &Ledger::get_end_time)
      .def("get_home_uid", &Ledger::get_home_uid)
      .def("get_home_uname", &Ledger::get_home_uname)
      .def("now", &Ledger::now)
      .def("run", &Ledger::run)
      .def("pre_setup", &Ledger::pre_setup)
      .def("setup", &Ledger::setup)
      .def("step", &Ledger::step)
      .def("is_live", &Ledger::is_live)
      .def("on_exit", &Ledger::on_exit);
}
} // namespace kungfu::wingchun::pybind
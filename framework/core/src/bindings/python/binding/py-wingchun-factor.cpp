// SPDX-License-Identifier: Apache-2.0

#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>

#include <kungfu/wingchun/factor/crosssection.h>

using namespace kungfu::longfist;
using namespace kungfu::longfist::types;
using namespace kungfu::longfist::enums;
using namespace kungfu::yijinjing;
using namespace kungfu::yijinjing::data;
using namespace kungfu::yijinjing::journal;
using namespace kungfu::wingchun;
using namespace kungfu::wingchun::factor;

namespace py = pybind11;
namespace kungfu::wingchun::pybind {

void bind_factor(pybind11::module &m) {

  py::class_<CrossSection, std::shared_ptr<CrossSection>>(m, "CrossSection")
      .def_static("loads", &CrossSection::loads, py::arg("serialized_cross_section"))
      .def_static("dumps", &CrossSection::dumps, py::arg("cross_section"))
      .def(py::init<>())
      // .def(py::init<std::unordered_map<std::string, float> , std::unordered_map<std::string, float>, int64_t>(
      //      py::arg("factors"), py::arg("prices"), py::arg("gen_time")))
      .def_readonly("gen_time", &CrossSection::gen_time, py::return_value_policy::reference)
      .def_readonly("factors", &CrossSection::factors, py::return_value_policy::reference)
      .def_readonly("prices", &CrossSection::prices, py::return_value_policy::reference)
      .def("__repr__", &CrossSection::to_string)
      .def("__parse__", [&](CrossSection &target, std::string &s) { target.from_string(s); });


class PyMultiCrossSectionalFactor : public MultiCrossSectionalFactor {
  using MultiCrossSectionalFactor::MultiCrossSectionalFactor;
protected:
  void on_quote(const Quote &quote) override { PYBIND11_OVERLOAD(void, MultiCrossSectionalFactor, on_quote, quote); }

  void on_tree(const Tree &tree) override { PYBIND11_OVERLOAD(void, MultiCrossSectionalFactor, on_tree, tree); }

  void on_depth(const Depth &depth) override { PYBIND11_OVERLOAD(void, MultiCrossSectionalFactor, on_depth, depth); }

  void on_tick(const Tick &tick) override { PYBIND11_OVERLOAD(void, MultiCrossSectionalFactor, on_tick, tick); }

  void on_entrust(const Entrust &entrust) override { PYBIND11_OVERLOAD(void, MultiCrossSectionalFactor, on_entrust, entrust); }

  void on_transaction(const Transaction &transaction) override {
    PYBIND11_OVERLOAD(void, MultiCrossSectionalFactor, on_transaction, transaction);
  }
};

  py::class_<MultiCrossSectionalFactor, PyMultiCrossSectionalFactor, std::shared_ptr<MultiCrossSectionalFactor>>(m, "MultiCrossSectionalFactor")
      .def(py::init<>())
      // .def("on_quote", &MultiCrossSectionalFactor::on_quote)
      // .def("on_tree", &MultiCrossSectionalFactor::on_tree)
      // .def("on_depth", &MultiCrossSectionalFactor::on_depth)
      // .def("on_tick", &MultiCrossSectionalFactor::on_tick)
      // .def("on_entrust", &MultiCrossSectionalFactor::on_entrust)
      // .def("on_transaction", &MultiCrossSectionalFactor::on_transaction)
      .def("generate_cross_sectional_factor", &MultiCrossSectionalFactor::generate_cross_sectional_factor)
      .def("update_price", &MultiCrossSectionalFactor::update_price)
      .def("update_factor", &MultiCrossSectionalFactor::update_factor);
}
} // namespace kungfu::wingchun::pybind
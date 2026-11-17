#include "python_baseline.hpp"

#include <pybind11/numpy.h>
#include <pybind11/stl.h>

#include <stdexcept>

namespace py = pybind11;

namespace seam {
namespace {

py::scoped_interpreter start_interpreter() {
    PyConfig config;
    PyConfig_InitPythonConfig(&config);
    config.parse_argv = 0;
    config.install_signal_handlers = 0;
    // On Windows, use the selected Python installation to locate its standard library.
    PyStatus status = PyConfig_SetBytesString(&config, &config.home, SEAM_PYTHON_HOME);
    if (PyStatus_Exception(status)) {
        PyConfig_Clear(&config);
        throw std::runtime_error("Could not configure the comparison interpreter");
    }
    return py::scoped_interpreter(&config, 0, nullptr, false);
}

} // namespace

PythonBaseline::PythonBaseline() : interpreter_(start_interpreter()) {
    try {
        py::list paths = py::module_::import("sys").attr("path");
        paths.attr("insert")(0, SEAM_NETWORKX_SITE);
        paths.attr("insert")(0, SEAM_NUMPY_SITE);
        // Import the existing module directly, without loading the legacy package.
        paths.attr("insert")(0, SEAM_LEGACY_ALGORITHMS);
        find_seam_ = py::module_::import("graphcut").attr("find_seam");
    } catch (const py::error_already_set& error) {
        throw std::runtime_error(error.what());
    }
}

Seam PythonBaseline::find(const EnergyView& energy) const {
    try {
        // The view presents either axis as rows of steps and columns of positions.
        py::array_t<double> input({energy.steps(), energy.positions()});
        auto values = input.mutable_unchecked<2>();
        for (int step = 0; step < energy.steps(); ++step) {
            for (int position = 0; position < energy.positions(); ++position) {
                values(step, position) = energy(step, position);
            }
        }

        return find_seam_(input).attr("tolist")().cast<Seam>();
    } catch (const py::error_already_set& error) {
        throw std::runtime_error(error.what());
    }
}

} // namespace seam

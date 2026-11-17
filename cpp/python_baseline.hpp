#pragma once

#include "seam/image.hpp"

#include <pybind11/embed.h>

namespace seam {

// Optional comparison adapter; construct before starting evaluation timers.
class PythonBaseline {
public:
    PythonBaseline();
    Seam find(const EnergyView& energy) const;

private:
    // Python objects must be destroyed before the interpreter is finalized.
    pybind11::scoped_interpreter interpreter_;
    pybind11::object find_seam_;
};

} // namespace seam

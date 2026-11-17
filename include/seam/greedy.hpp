#pragma once

#include "seam/image.hpp"

namespace seam {

Seam find_greedy(const EnergyView& energy);
Seam find_greedy_look3(const EnergyView& energy);

} // namespace seam

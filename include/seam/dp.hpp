#pragma once

#include "seam/image.hpp"

namespace seam {

// Positions follow the view's seam direction, one position per step.
Seam find_dp(const EnergyView& energy);
Seam find_dp_full(const EnergyView& energy);
// The returned table is indexed by (step, position), regardless of image axis.
Seam find_dp_full(const EnergyView& energy, Energy& cumulative);

} // namespace seam

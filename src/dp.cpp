#include "dp.hpp"

#include <algorithm>
#include <cstdint>
#include <vector>

namespace seam {
namespace {

// Each cost is the pixel energy plus the cheapest valid predecessor.
// Both storage modes use this same fill and traceback; ties keep the lowest index.
template <class Row> Seam solve(const EnergyView& energy, Row row) {
    const int steps = energy.steps();
    const int positions = energy.positions();
    std::vector<std::int8_t> parents(static_cast<std::size_t>(steps) * positions, 0);

    double* first = row(0);
    for (int position = 0; position < positions; ++position) {
        first[position] = energy(0, position);
    }

    for (int step = 1; step < steps; ++step) {
        const double* previous = row(step - 1);
        double* current = row(step);
        for (int position = 0; position < positions; ++position) {
            int best = std::max(0, position - 1);
            const int last = std::min(positions - 1, position + 1);
            for (int candidate = best + 1; candidate <= last; ++candidate) {
                if (previous[candidate] < previous[best]) {
                    best = candidate;
                }
            }
            current[position] = energy(step, position) + previous[best];
            parents[static_cast<std::size_t>(step) * positions + position] =
                static_cast<std::int8_t>(best - position);
        }
    }

    const double* last = row(steps - 1);
    int endpoint = 0;
    for (int position = 1; position < positions; ++position) {
        if (last[position] < last[endpoint]) {
            endpoint = position;
        }
    }

    Seam seam(steps);
    seam[steps - 1] = endpoint;
    for (int step = steps - 1; step > 0; --step) {
        int position = seam[step];
        seam[step - 1] = position + parents[static_cast<std::size_t>(step) * positions + position];
    }
    return seam;
}

} // namespace

Seam find_dp(const EnergyView& energy) {
    // Time O(steps * positions); costs use two rows, parents use one byte per pixel.
    std::vector<double> costs(static_cast<std::size_t>(2) * energy.positions());
    return solve(energy, [&](int step) {
        return costs.data() + static_cast<std::size_t>(step % 2) * energy.positions();
    });
}

Seam find_dp_full(const EnergyView& energy) {
    Energy cumulative;
    return find_dp_full(energy, cumulative);
}

Seam find_dp_full(const EnergyView& energy, Energy& cumulative) {
    // Keep every cost for inspection, in addition to the traceback parents.
    Energy table(energy.steps(), energy.positions());
    Seam seam = solve(energy, [&](int step) { return table.ptr<double>(step); });
    cumulative = table;
    return seam;
}

} // namespace seam

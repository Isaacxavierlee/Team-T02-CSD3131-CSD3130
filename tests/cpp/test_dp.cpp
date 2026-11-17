#include "check.hpp"
#include "seam/dp.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

using namespace seam;
using test::check;

namespace {

// Try every valid path on small maps, independently of the DP recurrence.
double enumerate(const EnergyView& map, int step, int position) {
    const double energy = map(step, position);
    if (step + 1 == map.steps()) {
        return energy;
    }

    double cheapest = std::numeric_limits<double>::infinity();
    for (int next = std::max(0, position - 1); next <= std::min(map.positions() - 1, position + 1);
         ++next) {
        cheapest = std::min(cheapest, enumerate(map, step + 1, next));
    }
    return energy + cheapest;
}

void compare_with_enumeration(const EnergyView& map) {
    Seam path = find_dp(map);
    check(path == find_dp_full(map), "compact and full DP disagree on the traced seam");
    check(static_cast<int>(path.size()) == map.steps(), "wrong number of seam pixels");

    double total = 0;
    for (int step = 0; step < map.steps(); ++step) {
        int position = path[step];
        check(position >= 0 && position < map.positions(), "seam leaves the image");
        check(step == 0 || std::abs(position - path[step - 1]) <= 1, "disconnected seam");
        total += map(step, position);
    }

    double optimum = std::numeric_limits<double>::infinity();
    for (int start = 0; start < map.positions(); ++start) {
        optimum = std::min(optimum, enumerate(map, 0, start));
    }
    check(total == optimum, "DP cost differs from exhaustive path enumeration");
}

void check_cumulative_values() {
    // This table can be checked by hand before testing the recovered path.
    Energy map = (Energy(3, 3) << 8, 1, 9, 7, 4, 2, 9, 5, 1);
    Energy expected = (Energy(3, 3) << 8, 1, 9, 8, 5, 3, 14, 8, 4);
    Energy table;
    Seam path = find_dp_full(EnergyView(map, Axis::Vertical), table);
    check(cv::norm(expected, table, cv::NORM_INF) == 0, "unexpected cumulative energy table");
    check(path == Seam({1, 2, 2}), "wrong backtrack from the cheapest endpoint");

    Energy flat(4, 6, 2.0);
    for (Axis axis : {Axis::Vertical, Axis::Horizontal}) {
        EnergyView view(flat, axis);
        check(find_dp(view) == Seam(view.steps(), 0), "ties must pick the first position");
    }
}

} // namespace

int main() {
    return test::run([] {
        check_cumulative_values();

        // Half-unit values are exact in binary, so cost equality is meaningful.
        cv::RNG random(73);
        for (int height = 1; height <= 5; ++height) {
            for (int width = 1; width <= 5; ++width) {
                for (int sample = 0; sample < 12; ++sample) {
                    Energy energy(height, width);
                    for (double& value : energy) {
                        value = 0.5 * random.uniform(0, 9);
                    }
                    for (Axis axis : {Axis::Vertical, Axis::Horizontal}) {
                        compare_with_enumeration(EnergyView(energy, axis));
                    }
                }
            }
        }
    });
}

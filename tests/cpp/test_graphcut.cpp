#include "check.hpp"
#include "seam/graphcut.hpp"
#include "seam/image.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

using namespace seam;
using test::check;

namespace {

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

void compare_with_enumeration(GraphCutSeamCarver& carver, const EnergyView& map) {
    std::vector<std::vector<double>> energies(
        map.steps(), std::vector<double>(map.positions()));
    for (int step = 0; step < map.steps(); ++step) {
        for (int position = 0; position < map.positions(); ++position) {
            energies[step][position] = map(step, position);
        }
    }

    const auto path = carver.find_vertical_seam(energies);
    check(static_cast<int>(path.size()) == map.steps(), "wrong number of seam pixels");

    double total = 0;
    for (int step = 0; step < map.steps(); ++step) {
        const int position = path[step];
        check(position >= 0 && position < map.positions(), "seam leaves the image");
        check(step == 0 || std::abs(position - path[step - 1]) <= 1, "disconnected seam");
        total += map(step, position);
    }

    double optimum = std::numeric_limits<double>::infinity();
    for (int start = 0; start < map.positions(); ++start) {
        optimum = std::min(optimum, enumerate(map, 0, start));
    }
    check(total == optimum, "graph-cut cost differs from exhaustive path enumeration");
}

void check_boundaries_and_capacities(GraphCutSeamCarver& carver) {
    const std::vector<std::vector<double>> last_column{{100, 0}, {100, 0}};
    check(carver.find_vertical_seam(last_column) == std::vector<int>({1, 1}),
          "did not choose the cheapest seam in the last column");

    const std::vector<std::vector<double>> small_values{{0.00005, 0}, {0.00005, 0}};
    check(carver.find_vertical_seam(small_values) == std::vector<int>({1, 1}),
          "ignored a small positive energy");

    const std::vector<std::vector<double>> large_values{{3e9, 2e9}, {3e9, 2e9}};
    check(carver.find_vertical_seam(large_values) == std::vector<int>({1, 1}),
          "constraint capacity was too small for the energies");

    check(carver.find_vertical_seam({}).empty(), "empty energy map should return an empty seam");
    test::rejects([&] { carver.find_vertical_seam({{}}); }, "empty row was accepted");
    test::rejects([&] { carver.find_vertical_seam({{1, 2}, {1}}); },
                  "non-rectangular energy map was accepted");
    test::rejects([&] { carver.find_vertical_seam({{-1}}); }, "negative energy was accepted");
    test::rejects(
        [&] { carver.find_vertical_seam({{std::numeric_limits<double>::quiet_NaN()}}); },
        "non-finite energy was accepted");
}

} // namespace

int main() {
    return test::run([] {
        GraphCutSeamCarver carver;
        check_boundaries_and_capacities(carver);

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
                        compare_with_enumeration(carver, EnergyView(energy, axis));
                    }
                }
            }
        }
    });
}

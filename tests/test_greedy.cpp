#include "check.hpp"
#include "greedy.hpp"

#include <algorithm>
#include <cmath>

using namespace seam;
using test::check;

namespace {

void check_local_decisions(const EnergyView& energy) {
    const Seam path = find_greedy(energy);
    check(static_cast<int>(path.size()) == energy.steps(), "wrong seam length");
    for (int step = 0; step < energy.steps(); ++step) {
        // The first choice is unrestricted; later choices must touch the previous pixel.
        int first = step == 0 ? 0 : std::max(0, path[step - 1] - 1);
        int last = step == 0 ? energy.positions() - 1
                             : std::min(energy.positions() - 1, path[step - 1] + 1);
        check(path[step] >= first && path[step] <= last, "greedy seam leaves its neighbourhood");
        for (int candidate = first; candidate <= last; ++candidate) {
            double chosen = energy(step, path[step]);
            double other = energy(step, candidate);
            check(chosen < other || (chosen == other && path[step] <= candidate),
                  "greedy did not choose the first local minimum");
        }
    }
}

void check_decoy() {
    // The tempting zero leads to expensive pixels; column two stays cheap.
    Energy decoy =
        (Energy(4, 5) << 0, 9, 1, 9, 9, 50, 50, 1, 50, 50, 50, 50, 1, 50, 50, 50, 50, 1, 50, 50);
    Energy sideways = decoy.t();
    for (auto view : {EnergyView(decoy, Axis::Vertical), EnergyView(sideways, Axis::Horizontal)}) {
        check(find_greedy(view) == Seam({0, 0, 0, 0}), "plain greedy should take the decoy");
        check(find_greedy_look3(view) == Seam({2, 2, 2, 2}), "lookahead should see the cheap path");
    }
}

} // namespace

int main() {
    return test::run([] {
        check_decoy();
        cv::RNG random(31);
        for (int height = 1; height <= 7; ++height) {
            for (int width = 1; width <= 9; ++width) {
                Energy map(height, width);
                for (double& value : map) {
                    value = random.uniform(0, 4);
                }
                for (Axis axis : {Axis::Vertical, Axis::Horizontal}) {
                    check_local_decisions(EnergyView(map, axis));
                }
            }
        }
    });
}

#include "check.hpp"
#include "python_baseline.hpp"
#include "seam/dp.hpp"
#include "seam/greedy.hpp"

#include <cmath>

using namespace seam;
using test::check;

namespace {

double cost(const EnergyView& energy, const Seam& path) {
    check(static_cast<int>(path.size()) == energy.steps(), "baseline seam length");
    double sum = 0;
    for (int step = 0; step < energy.steps(); ++step) {
        check(path[step] >= 0 && path[step] < energy.positions(), "baseline leaves the image");
        check(step == 0 || std::abs(path[step] - path[step - 1]) <= 1, "disconnected baseline");
        sum += energy(step, path[step]);
    }
    return sum;
}

} // namespace

int main() {
    return test::run([] {
        PythonBaseline graphcut;
        cv::RNG random(11);
        for (int rows = 1; rows <= 5; ++rows) {
            for (int cols = 1; cols <= 5; ++cols) {
                Energy map(rows, cols);
                for (double& value : map) {
                    value = 0.5 * random.uniform(0, 9);
                }
                // Both directions use the same values, viewed in the seam's orientation.
                for (Axis axis : {Axis::Vertical, Axis::Horizontal}) {
                    EnergyView view(map, axis);
                    double optimum = cost(view, find_dp(view));
                    // Equal-cost paths may differ when several seams tie for the minimum.
                    check(cost(view, graphcut.find(view)) == optimum,
                          "graph-cut cost differs from DP");
                    check(cost(view, find_greedy(view)) >= optimum,
                          "greedy cost below the optimum");
                }
            }
        }
    });
}

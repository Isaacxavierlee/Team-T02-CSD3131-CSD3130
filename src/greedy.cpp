#include "greedy.hpp"

#include <algorithm>

namespace seam {
namespace {

// Score this pixel and the cheapest continuation through up to three more steps.
double lookahead_cost(const EnergyView& energy, int step, int position, int remaining) {
    double cost = energy(step, position);
    if (remaining == 0 || step + 1 == energy.steps()) {
        return cost;
    }

    int first = std::max(0, position - 1);
    int last = std::min(energy.positions() - 1, position + 1);
    double best = lookahead_cost(energy, step + 1, first, remaining - 1);
    for (int next = first + 1; next <= last; ++next) {
        best = std::min(best, lookahead_cost(energy, step + 1, next, remaining - 1));
    }
    return cost + best;
}

// Build a connected seam; a strict comparison keeps the earliest position on ties.
template <class Score> Seam choose_path(const EnergyView& energy, Score score) {
    Seam path(energy.steps());
    for (int step = 0; step < energy.steps(); ++step) {
        int first = step == 0 ? 0 : std::max(0, path[step - 1] - 1);
        int last = step == 0 ? energy.positions() - 1
                             : std::min(energy.positions() - 1, path[step - 1] + 1);
        int best = first;
        double best_cost = score(step, first);
        for (int position = first + 1; position <= last; ++position) {
            double cost = score(step, position);
            if (cost < best_cost) {
                best = position;
                best_cost = cost;
            }
        }
        path[step] = best;
    }
    return path;
}

} // namespace

Seam find_greedy(const EnergyView& energy) {
    // Local choices take O(positions + steps) time, but need not be globally optimal.
    return choose_path(energy, [&](int step, int position) { return energy(step, position); });
}

Seam find_greedy_look3(const EnergyView& energy) {
    // Look ahead three additional steps before committing each local choice.
    return choose_path(
        energy, [&](int step, int position) { return lookahead_cost(energy, step, position, 3); });
}

} // namespace seam

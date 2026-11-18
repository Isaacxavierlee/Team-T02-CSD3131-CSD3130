#include "dp.hpp"

#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

using namespace seam;
constexpr int repetitions = 5;

template <class Operation> double mean_time(Operation operation) {
    const auto start = std::chrono::steady_clock::now();
    for (int repeat = 0; repeat < repetitions; ++repeat) {
        operation();
    }
    const auto elapsed = std::chrono::steady_clock::now() - start;
    return std::chrono::duration<double, std::milli>(elapsed).count() / repetitions;
}

void measure(const Image& image) {
    const Energy reference = compute_energy(image);
    const EnergyView view(reference, Axis::Vertical);
    const Seam expected = find_dp(view);
    const auto prefix = std::to_string(image.cols) + "," + std::to_string(image.rows) + ",";

    // Verify the strip result before timing the same computation repeatedly.
    for (int workers : {1, 2, 4, 8}) {
        auto method = workers == 1 ? EnergyMethod::Sobel : EnergyMethod::SobelStrips;
        Energy result = compute_energy(image, method, Axis::Vertical, workers);
        if (cv::norm(reference, result, cv::NORM_INF) != 0) {
            throw std::runtime_error("Strip boundaries changed the Sobel result");
        }
        double time =
            mean_time([&] { result = compute_energy(image, method, Axis::Vertical, workers); });
        std::cout << prefix << "sobel," << workers << "_worker," << repetitions << ',' << time
                  << ",\n";
    }

    for (bool full : {false, true}) {
        auto run = [&] { return full ? find_dp_full(view) : find_dp(view); };
        Seam result = run();
        if (result != expected) {
            throw std::runtime_error("DP storage modes produced different paths");
        }
        double time = mean_time([&] { result = run(); });
        // Count cost and parent buffers, not total process or peak memory.
        const auto cost_cells = full ? image.total() : 2 * static_cast<std::size_t>(image.cols);
        const auto bytes = cost_cells * sizeof(double) + image.total() * sizeof(std::int8_t);
        std::cout << prefix << "vertical_dp," << (full ? "full" : "rolling") << ',' << repetitions
                  << ',' << time << ',' << bytes << '\n';
    }
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: seam-profile IMAGE\n";
        return 2;
    }
    try {
        const seam::Image original = seam::read_image(argv[1]);
        std::cout << std::fixed << std::setprecision(6)
                  << "width,height,stage,variant,repetitions,mean_ms,dp_work_buffer_bytes\n";
        for (int width : {64, 128, 256, 512}) {
            measure(downscale_to_width(original, width));
            if (width >= original.cols) {
                break;
            }
        }
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

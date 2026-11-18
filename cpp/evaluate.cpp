#include "arguments.hpp"
#include "seam/dp.hpp"
#include "seam/greedy.hpp"
#include "seam/resize.hpp"
#include "seam/graphcut.hpp"

#ifdef SEAM_WITH_PYTHON_GRAPHCUT
#include "python_baseline.hpp"
#endif

#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using namespace seam;
namespace fs = std::filesystem;
using Clock = std::chrono::steady_clock;

struct Options {
    fs::path out = "output/eval";
    int max_width = 48, cols = 4, rows = 3, repeats = 3;
    bool graphcut = false;
    std::vector<fs::path> images;
};

Options parse(int argc, char** argv) {
    Options options;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--with-graphcut") {
            options.graphcut = true;
        } else if (arg == "--out" || arg == "--max-width" || arg == "--cols" || arg == "--rows" ||
                   arg == "--repeats") {
            if (++i == argc || std::string(argv[i]).rfind("--", 0) == 0) {
                throw std::invalid_argument("Missing value for " + arg);
            }
            std::string value = argv[i];
            if (arg == "--out") {
                if (value.empty()) {
                    throw std::invalid_argument("Output directory is empty");
                }
                options.out = value;
            } else if (arg == "--max-width") {
                options.max_width = seam_app::integer_option(arg, value, 1);
            } else if (arg == "--cols") {
                options.cols = seam_app::integer_option(arg, value, 0);
            } else if (arg == "--rows") {
                options.rows = seam_app::integer_option(arg, value, 0);
            } else {
                options.repeats = seam_app::integer_option(arg, value, 1);
            }
        } else if (!arg.empty() && arg[0] == '-') {
            throw std::invalid_argument("Unknown option: " + arg);
        } else {
            options.images.emplace_back(arg);
        }
    }
    if (options.images.empty()) {
        throw std::invalid_argument("Provide at least one image path");
    }
    return options;
}

struct Method {
    std::string name, implementation;
    Finder finder;
};

struct Input {
    fs::path path;
    Image preview;
    cv::Size target;
};

struct Result {
    Image image, overlay;
    std::vector<Seam> paths;
    Stats stats;
};

Result run(const Image& input, cv::Size target, const Finder& finder, bool tracking) {
    ResizeSession session(input, target, finder, EnergyMethod::Sobel, Order::VerticalFirst,
                          tracking);
    Result result;
    while (session.phase() != Phase::Complete) {
        session.advance();
        if (tracking) {
            result.paths.push_back(session.seam());
        }
        session.advance();
    }
    result.image = session.image();
    result.overlay = session.overlay();
    result.stats = session.stats();
    return result;
}

std::string csv(const std::string& value) {
    std::string escaped = "\"";
    for (char ch : value) {
        if (ch == '"') {
            escaped += '"';
        }
        escaped += ch;
    }
    return escaped + '"';
}

double difference(const Image& a, const Image& b) {
    return cv::norm(a, b, cv::NORM_L1) / (static_cast<double>(a.total()) * 3.0);
}

std::size_t different_paths(const std::vector<Seam>& a, const std::vector<Seam>& b) {
    if (a.size() != b.size()) {
        throw std::runtime_error("Methods removed different seam counts");
    }
    std::size_t count = 0;
    for (std::size_t i = 0; i < a.size(); ++i) {
        count += a[i] != b[i];
    }
    return count;
}

// All cells share a scale; the extra width keeps labels readable on tiny inputs.
void save_panel(const fs::path& path, const Image& input, const std::vector<Method>& methods,
                const std::vector<Result>& results) {
    const double scale = std::max(1.0, 175.0 / std::max(input.cols, input.rows));
    constexpr double font_size = 0.45;
    constexpr int gutter = 6;
    int width = std::max(110, cvRound(input.cols * scale)) + 2 * gutter;
    for (const auto& method : methods) {
        int baseline = 0;
        auto text_size = cv::getTextSize(method.name + " resized", cv::FONT_HERSHEY_SIMPLEX,
                                         font_size, 1, &baseline);
        width = std::max(width, text_size.width + 2 * gutter);
    }
    const int height = std::max(1, cvRound(input.rows * scale));
    const int label = 26;
    Image panel(2 * (height + label), static_cast<int>(methods.size() + 1) * width,
                cv::Vec3b(255, 255, 255));
    auto cell = [&](int column, int row, const std::string& title, const Image& picture) {
        int x = column * width, y = row * (height + label);
        cv::putText(panel, title, cv::Point(x + gutter, y + 18), cv::FONT_HERSHEY_SIMPLEX,
                    font_size, cv::Scalar(0, 0, 0), 1, cv::LINE_AA);
        Image scaled;
        cv::resize(picture, scaled,
                   cv::Size(std::max(1, cvRound(picture.cols * scale)),
                            std::max(1, cvRound(picture.rows * scale))),
                   0, 0, cv::INTER_NEAREST);
        scaled.copyTo(panel(cv::Rect(x + gutter, y + label, scaled.cols, scaled.rows)));
    };
    cell(0, 0, "Original", input);
    Image energy_preview;
    cv::cvtColor(energy_image(compute_energy(input)), energy_preview, cv::COLOR_GRAY2BGR);
    cell(0, 1, "Sobel energy", energy_preview);
    for (std::size_t i = 0; i < methods.size(); ++i) {
        cell(static_cast<int>(i + 1), 0, methods[i].name + " resized", results[i].image);
        cell(static_cast<int>(i + 1), 1, methods[i].name + " seams", results[i].overlay);
    }
    save_image(path, panel);
}

std::vector<Input> load_inputs(const Options& options) {
    // Validate every input and target before writing any output.
    std::vector<Input> inputs;
    std::set<std::string> stems;
    for (const auto& path : options.images) {
        std::string stem = path.stem().string();
        if (stem.empty() || !stems.insert(stem).second) {
            throw std::invalid_argument("Empty or duplicate image stem: " + stem);
        }
        Image preview = downscale_to_width(read_image(path), options.max_width);
        if (options.cols >= preview.cols || options.rows >= preview.rows) {
            throw std::invalid_argument(
                "Target must retain at least one pixel in each dimension: " + path.string());
        }
        inputs.push_back(
            {path, preview, {preview.cols - options.cols, preview.rows - options.rows}});
    }
    return inputs;
}

void measure_image(const Input& input, const Options& options, const std::vector<Method>& methods,
                   std::ostream& report) {
    fs::path folder = options.out / input.path.stem();
    fs::create_directories(folder);
    save_image(folder / "input.png", input.preview);
    std::vector<Result> results;
    for (const Method& method : methods) {
        // The tracked pass warms the method and records paths for comparison.
        results.push_back(run(input.preview, input.target, method.finder, true));
        const Result& reference = results.back();
        save_image(folder / (method.name + "_resized.png"), reference.image);
        save_image(folder / (method.name + "_seams.png"), reference.overlay);
        double diff = difference(results.front().image, reference.image);
        std::size_t paths = different_paths(results.front().paths, reference.paths);
        for (int repetition = 1; repetition <= options.repeats; ++repetition) {
            // Time an untracked run; image and cost must match the tracked pass.
            auto start = Clock::now();
            Result timed = run(input.preview, input.target, method.finder, false);
            auto end = Clock::now();
            if (cv::norm(reference.image, timed.image, cv::NORM_INF) != 0 ||
                reference.stats.seam_cost != timed.stats.seam_cost) {
                throw std::runtime_error("Inconsistent repeated result for " + method.name);
            }
            double total = std::chrono::duration<double, std::milli>(end - start).count();
            report << csv(input.path.string()) << ',' << input.preview.cols << ','
                   << input.preview.rows << ',' << input.target.width << ',' << input.target.height
                   << ',' << csv(method.name) << ',' << csv(method.implementation) << ','
                   << csv("width-limited INTER_AREA") << ',' << repetition << ','
                   << timed.stats.energy_ms << ',' << timed.stats.seam_ms << ','
                   << timed.stats.remove_ms << ',' << total << ',' << timed.stats.seam_cost << ','
                   << diff << ',' << paths << '\n';
        }
    }
    save_panel(folder / "comparison.png", input.preview, methods, results);
}

} // namespace

int main(int argc, char** argv) {
    try {
        if (argc == 2 && std::string(argv[1]) == "--help") {
            std::cout << "Usage: seam-evaluate [--out DIR] [--max-width N] [--cols N] [--rows N] "
                         "[--repeats N] [--with-graphcut] IMAGE...\n";
            return 0;
        }
        const Options options = parse(argc, argv);
#ifndef SEAM_WITH_PYTHON_GRAPHCUT
        if (options.graphcut) {
            throw std::runtime_error(
                "--with-graphcut requires a build with Python graph-cut support");
        }
#endif
        const auto inputs = load_inputs(options);

#ifdef SEAM_WITH_PYTHON_GRAPHCUT
        std::unique_ptr<PythonBaseline> baseline;
        if (options.graphcut) {
            baseline = std::make_unique<PythonBaseline>();
        }
#endif
        std::vector<Method> methods{{"dp", "C++", find_dp}, {"greedy", "C++", find_greedy}};
        
        methods.push_back({"graphcut", "C++", [&](const EnergyView& view) {
            int H = view.steps();
            int W = view.positions();
            
            std::vector<std::vector<double>> energy_matrix(H, std::vector<double>(W));
            for (int r = 0; r < H; ++r) {
                for (int c = 0; c < W; ++c) {
                    energy_matrix[r][c] = view(r, c);
                }
            }
            GraphCutSeamCarver gc;
            return gc.find_vertical_seam(energy_matrix);
        }});

#ifdef SEAM_WITH_PYTHON_GRAPHCUT
        if (baseline) {
            methods.push_back({"graphcut", "Python (Dinic)",
                               [&](const EnergyView& view) { return baseline->find(view); }});
        }
#endif
        fs::create_directories(options.out);
        std::ofstream report(options.out / "results.csv");
        if (!report) {
            throw std::runtime_error("Could not open results.csv");
        }
        report << "image,width,height,target_width,target_height,method,implementation,prep,"
                  "repetition,energy_ms,search_ms,remove_ms,total_ms,seam_cost,mean_abs_bgr_vs_dp,"
                  "different_paths_vs_dp\n";
        report << std::fixed << std::setprecision(6);
        for (const Input& input : inputs) {
            measure_image(input, options, methods, report);
        }
        report.close();
        if (!report) {
            throw std::runtime_error("Could not write results.csv");
        }
        std::cout << "Wrote " << (options.out / "results.csv").string() << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "seam-evaluate: " << error.what() << '\n';
        return 1;
    }
}

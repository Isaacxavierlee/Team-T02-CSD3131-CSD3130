#include "app.hpp"

#include "arguments.hpp"
#include "dp.hpp"
#include "greedy.hpp"
#include <algorithm>
#include <charconv>
#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <opencv2/videoio.hpp>
#include <sstream>
#include <stdexcept>

namespace seam_app {

using namespace seam;
namespace fs = std::filesystem;

const std::vector<Algorithm>& algorithms() {
    static const std::vector<Algorithm> choices{
        {"dp", find_dp, true},
        {"dp_full", [](const EnergyView& view) { return find_dp_full(view); }, true},
        {"dp_lowmem", find_dp, false},
        {"dp_loop", [](const EnergyView& view) { return find_dp_full(view); }, false},
        {"greedy", find_greedy, true},
        {"greedy_look3", find_greedy_look3, true}};
    return choices;
}

const std::vector<EnergyChoice>& energies() {
    static const std::vector<EnergyChoice> choices{{"sobel", EnergyMethod::Sobel},
                                                   {"gradient", EnergyMethod::Gradient},
                                                   {"forward", EnergyMethod::Forward},
                                                   {"sobel_dc", EnergyMethod::SobelStrips}};
    return choices;
}

Finder finder(const std::string& name) {
    for (const auto& choice : algorithms()) {
        if (choice.name == name) {
            return choice.run;
        }
    }
    throw std::invalid_argument("Unknown algorithm: " + name);
}

void save_outputs(const Options& options, const Image& source, const ResizeSession& session) {
    const std::string prefix = options.out.string();
    save_image(prefix + "_" + options.algo + ".png", session.image());
    save_image(prefix + "_" + options.algo + "_seams.png", session.overlay());
    save_image(prefix + "_energy.png", energy_image(compute_energy(source, options.energy)));
}

Options parse(int argc, char** argv) {
    Options options;
    options.interactive = argc == 1;
    bool image_set = false;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help") {
            std::cout
                << "Usage: seam-carve [IMAGE] [--interactive] [--width N|--remove-cols N] "
                   "[--height N|--remove-rows N]\n"
                   "  [--max-width N] [--algo dp|dp_full|dp_lowmem|dp_loop|greedy|greedy_look3]\n"
                   "  [--energy sobel|gradient|forward|sobel_dc] [--order "
                   "vertical_first|alternate]\n"
                   "  [--out PREFIX] [--frames-dir DIR] [--video FILE] [--gui-smoke]\n";
            std::exit(0);
        }

        if (arg == "--interactive") {
            options.interactive = true;
            continue;
        }
        if (arg == "--gui-smoke") {
            options.smoke = options.interactive = true;
            continue;
        }
        if (arg.rfind("--", 0) == 0) {
            auto next_value = [&]() -> std::string {
                if (i + 1 >= argc || std::string(argv[i + 1]).rfind("--", 0) == 0) {
                    throw std::invalid_argument("Missing value for " + arg);
                }
                return argv[++i];
            };
            if (arg == "--width") {
                options.width = integer_option(arg, next_value(), 1);
            } else if (arg == "--height") {
                options.height = integer_option(arg, next_value(), 1);
            } else if (arg == "--remove-cols") {
                options.remove_cols = integer_option(arg, next_value(), 0);
            } else if (arg == "--remove-rows") {
                options.remove_rows = integer_option(arg, next_value(), 0);
            } else if (arg == "--max-width") {
                options.max_width = integer_option(arg, next_value(), 1);
            } else if (arg == "--algo") {
                options.algo = next_value();
            } else if (arg == "--out") {
                options.out = next_value();
            } else if (arg == "--frames-dir") {
                options.frames = next_value();
            } else if (arg == "--video") {
                options.video = next_value();
            } else if (arg == "--order") {
                const auto value = next_value();
                if (value == "vertical_first") {
                    options.order = Order::VerticalFirst;
                } else if (value == "alternate") {
                    options.order = Order::Alternate;
                } else {
                    throw std::invalid_argument("Unknown order: " + value);
                }
            } else if (arg == "--energy") {
                const auto value = next_value();
                const auto& choices = energies();
                auto match =
                    std::find_if(choices.begin(), choices.end(),
                                 [&](const EnergyChoice& choice) { return choice.name == value; });
                if (match == choices.end()) {
                    throw std::invalid_argument("Unknown energy: " + value);
                }
                options.energy = match->method;
            } else {
                throw std::invalid_argument("Unknown option: " + arg);
            }
        } else {
            if (image_set || (!arg.empty() && arg[0] == '-')) {
                throw std::invalid_argument("Unexpected argument: " + arg);
            }
            options.image = arg;
            image_set = true;
        }
    }
    if ((options.width != -1 && options.remove_cols != -1) ||
        (options.height != -1 && options.remove_rows != -1)) {
        throw std::invalid_argument("Choose either target size or removal count for each axis");
    }
    if (options.out.empty()) {
        throw std::invalid_argument("Output prefix cannot be empty");
    }
    finder(options.algo);
    if (options.interactive && (!options.frames.empty() || !options.video.empty())) {
        throw std::invalid_argument(
            "Interactive frame/video export is not supported; run batch mode instead");
    }
    return options;
}

namespace {

std::string frame_name(int step, const char* stage) {
    std::ostringstream name;
    name << "step_" << std::setfill('0') << std::setw(6) << step << '_' << stage << ".png";
    return name.str();
}

bool is_obsolete_frame(const std::string& name, int last_step) {
    if (name == "step_000000_original.png") {
        return last_step > 0;
    }
    if (name.compare(0, 5, "step_") != 0) {
        return false;
    }
    int step = 0;
    const auto parsed = std::from_chars(name.data() + 5, name.data() + name.size(), step);
    return parsed.ec == std::errc{} && step > last_step &&
           (name == frame_name(step, "highlighted") || name == frame_name(step, "removed"));
}

} // namespace

void batch(const Options& options, const Image& source, cv::Size target) {
    const auto start = std::chrono::steady_clock::now();
    ResizeSession session(source, target, finder(options.algo), options.energy, options.order,
                          true);
    cv::VideoWriter video;
    // Video encoders need a fixed, even-sized canvas as the image shrinks.
    cv::Size canvas(source.cols + source.cols % 2, source.rows + source.rows % 2);
    if (!options.video.empty()) {
        if (!options.video.parent_path().empty()) {
            fs::create_directories(options.video.parent_path());
        }
        video.open(options.video.string(), cv::VideoWriter::fourcc('m', 'p', '4', 'v'), 15, canvas);
        if (!video.isOpened()) {
            throw std::runtime_error("Cannot open video: " + options.video.string());
        }
    }

    int step = 0;
    auto frame = [&](const Image& image, const char* stage) {
        if (!options.frames.empty()) {
            save_image(options.frames / frame_name(step, stage), image);
        }
        if (video.isOpened()) {
            Image padded(canvas, cv::Vec3b(0, 0, 0));
            image.copyTo(padded(cv::Rect(0, 0, image.cols, image.rows)));
            video.write(padded);
        }
    };
    bool exporting = !options.frames.empty() || video.isOpened();
    if (session.phase() == Phase::Complete && exporting) {
        frame(source, "original");
    }
    // Each seam has a visible highlight phase followed by removal.
    while (session.phase() != Phase::Complete) {
        ++step;
        session.advance();
        if (exporting) {
            frame(session.highlighted(), "highlighted");
        }
        session.advance();
        if (exporting) {
            frame(session.image(), "removed");
        }
    }
    const double elapsed =
        std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    save_outputs(options, source, session);
    if (!options.frames.empty()) {
        // Remove leftover playback frames after the new images are saved.
        for (const auto& entry : fs::directory_iterator(options.frames)) {
            if (is_obsolete_frame(entry.path().filename().u8string(), step) &&
                fs::is_regular_file(entry.symlink_status())) {
                fs::remove(entry.path());
            }
        }
    }
    const Stats& stats = session.stats();
    std::cout << "Elapsed " << elapsed << " ms; energy " << stats.energy_ms << " ms; search "
              << stats.seam_ms << " ms; removal " << stats.remove_ms << " ms; seams "
              << stats.vertical << " vertical, " << stats.horizontal << " horizontal\n";
    std::cout << "Saved " << options.out.string() << "_" << options.algo << ".png; seam cost "
              << session.stats().seam_cost << '\n';
}
} // namespace seam_app

int main(int argc, char** argv) {
    try {
        auto options = seam_app::parse(argc, argv);
        auto source = seam::read_image(options.image);
        if (options.max_width) {
            source = seam::downscale_to_width(source, options.max_width);
        }
        // Targets and removal counts apply to the prepared image after downscaling.
        int width =
            options.width != -1 ? options.width : source.cols - std::max(0, options.remove_cols);
        int height =
            options.height != -1 ? options.height : source.rows - std::max(0, options.remove_rows);
        if (options.interactive && options.width == -1 && options.remove_cols == -1) {
            width = std::max(1, source.cols - std::max(1, source.cols / 10));
        }
        if (options.interactive && options.height == -1 && options.remove_rows == -1) {
            height = std::max(1, source.rows - std::max(1, source.rows / 10));
        }
        if (width < 1 || height < 1 || width > source.cols || height > source.rows) {
            throw std::invalid_argument("Target must be between 1x1 and the input dimensions");
        }
        if (options.interactive) {
            return seam_app::studio(options, source, {width, height});
        }
        seam_app::batch(options, source, {width, height});
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "seam-carve: " << e.what() << '\n';
        return 1;
    }
}

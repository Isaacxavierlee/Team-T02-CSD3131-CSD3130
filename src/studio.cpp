#include "app.hpp"
#include <algorithm>
#include <cctype>
#include <chrono>
#include <memory>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <stdexcept>
#include <string>
#include <vector>

namespace seam_app {
using namespace seam;

int studio(const Options& initial, const Image& source, cv::Size target) {
    const std::string window = "Seam Carving";
    std::vector<Algorithm> visible_algorithms;
    for (const auto& choice : algorithms()) {
        if (choice.gui) {
            visible_algorithms.push_back(choice);
        }
    }
    const auto& energy_choices = energies();
    Options options = initial;
    if (initial.smoke && source.cols < 2) {
        throw std::invalid_argument("GUI smoke test needs an image at least two pixels wide");
    }
    int width = initial.smoke ? source.cols : target.width;
    int height = initial.smoke ? source.rows : target.height;
    int algorithm = 0;
    for (std::size_t i = 0; i < visible_algorithms.size(); ++i) {
        if (visible_algorithms[i].name == options.algo ||
            (options.algo == "dp_lowmem" && visible_algorithms[i].name == "dp") ||
            (options.algo == "dp_loop" && visible_algorithms[i].name == "dp_full")) {
            algorithm = static_cast<int>(i);
        }
    }
    int energy = 0;
    for (std::size_t i = 0; i < energy_choices.size(); ++i) {
        if (energy_choices[i].method == options.energy) {
            energy = static_cast<int>(i);
        }
    }
    cv::namedWindow(window, cv::WINDOW_AUTOSIZE);
    cv::createTrackbar("Width", window, nullptr, source.cols);
    cv::createTrackbar("Height", window, nullptr, source.rows);
    cv::setTrackbarMin("Width", window, 1);
    cv::setTrackbarMin("Height", window, 1);
    cv::createTrackbar("Algorithm", window, nullptr,
                       static_cast<int>(visible_algorithms.size()) - 1);
    cv::createTrackbar("Energy", window, nullptr, static_cast<int>(energy_choices.size()) - 1);
    cv::setTrackbarPos("Width", window, width);
    cv::setTrackbarPos("Height", window, height);
    cv::setTrackbarPos("Algorithm", window, algorithm);
    cv::setTrackbarPos("Energy", window, energy);
    std::unique_ptr<ResizeSession> session;
    bool playing = false;
    auto restart = [&] {
        options.algo = visible_algorithms[algorithm].name;
        options.energy = energy_choices[energy].method;
        session =
            std::make_unique<ResizeSession>(source, cv::Size(width, height), finder(options.algo),
                                            options.energy, options.order, true);
        playing = true;
    };
    restart();
    int ticks = 0;
    bool saw_highlight = false, saw_removal = false, saw_change = false;
    auto started = std::chrono::steady_clock::now();
    int smoke_width = source.cols > 1 ? source.cols - 1 : 1;
    while (true) {
        int w = std::max(1, cv::getTrackbarPos("Width", window));
        int h = std::max(1, cv::getTrackbarPos("Height", window));
        int a = cv::getTrackbarPos("Algorithm", window);
        int e = cv::getTrackbarPos("Energy", window);
        // A new setting starts again from the source, not the partly carved image.
        if (w != width || h != height || a != algorithm || e != energy) {
            width = w;
            height = h;
            algorithm = a;
            energy = e;
            restart();
            if (options.smoke && width == smoke_width) {
                saw_change = true;
            }
        }
        if (playing && session->phase() != Phase::Complete) {
            session->advance();
        }
        if (options.smoke && saw_change) {
            if (session->phase() == Phase::Highlighted) {
                saw_highlight = true;
            }
            if (saw_highlight && session->image().cols < source.cols) {
                saw_removal = true;
            }
        }

        // Use one display scale for both views; text has its own fixed-height area.
        double scale = std::min(420.0 / source.cols, 320.0 / source.rows);
        int pw = std::max(1, cvRound(source.cols * scale));
        int ph = std::max(1, cvRound(source.rows * scale));
        int canvas_width = std::max(650, pw * 2 + 30);
        Image canvas(ph + 100, canvas_width, cv::Vec3b(245, 245, 245));
        auto draw = [&](const Image& img, int x) {
            Image scaled;
            cv::resize(
                img, scaled,
                {std::max(1, cvRound(img.cols * scale)), std::max(1, cvRound(img.rows * scale))});
            scaled.copyTo(canvas(cv::Rect(x, 90, scaled.cols, scaled.rows)));
        };
        cv::putText(canvas, "Space: play/pause   N: step   R: restart   S: save   Q/Esc: quit",
                    {8, 20}, cv::FONT_HERSHEY_SIMPLEX, .48, {0, 0, 0}, 1, cv::LINE_AA);
        cv::putText(canvas,
                    "Target " + std::to_string(width) + "x" + std::to_string(height) + "   " +
                        options.algo + " / " + energy_choices[energy].name +
                        (session->phase() == Phase::Complete
                             ? "  complete"
                             : (playing ? "  playing" : "  paused")) +
                        "  current " + std::to_string(session->image().cols) + "x" +
                        std::to_string(session->image().rows),
                    {8, 42}, cv::FONT_HERSHEY_SIMPLEX, .48, {0, 0, 0}, 1, cv::LINE_AA);
        cv::putText(canvas, "Original + seams", {8, 80}, cv::FONT_HERSHEY_SIMPLEX, .48, {0, 0, 0},
                    1, cv::LINE_AA);
        cv::putText(canvas, "Current", {pw + 20, 80}, cv::FONT_HERSHEY_SIMPLEX, .48, {0, 0, 0}, 1,
                    cv::LINE_AA);
        draw(session->overlay(), 0);
        draw(session->highlighted(), pw + 20);
        cv::imshow(window, canvas);
        if (options.smoke && ticks++ == 0) {
            cv::setTrackbarPos("Width", window, smoke_width);
        }
        if (options.smoke && saw_change && saw_highlight && saw_removal &&
            session->image().cols == smoke_width) {
            break;
        }
        if (options.smoke && std::chrono::steady_clock::now() - started > std::chrono::seconds(5)) {
            throw std::runtime_error("GUI smoke test did not complete slider and seam transitions");
        }
        const int key = std::tolower(cv::waitKey(30) & 0xff);
        if (key == 27 || key == 'q') {
            break;
        }
        if (key == ' ') {
            playing = !playing;
        }
        if (key == 'n') {
            playing = false;
            session->advance();
        }
        if (key == 'r') {
            restart();
        }
        if (key == 's') {
            save_outputs(options, source, *session);
        }
        if (cv::getWindowProperty(window, cv::WND_PROP_VISIBLE) < 1) {
            break;
        }
    }
    if (cv::getWindowProperty(window, cv::WND_PROP_VISIBLE) >= 1) {
        cv::destroyWindow(window);
    }
    if (options.smoke && !(saw_change && saw_highlight && saw_removal)) {
        throw std::runtime_error("The window closed before the slider and seam checks finished");
    }
    return 0;
}
} // namespace seam_app

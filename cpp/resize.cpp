#include "seam/resize.hpp"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <stdexcept>
#include <utility>

namespace seam {
namespace {

using Clock = std::chrono::steady_clock;

double milliseconds(Clock::time_point start, Clock::time_point end) {
    return std::chrono::duration<double, std::milli>(end - start).count();
}

// The same coordinate mapping removes pixels from the image and its tracker.
template <class Pixel>
cv::Mat_<Pixel> remove_path(const cv::Mat_<Pixel>& source, Axis axis, const Seam& path) {
    int rows = source.rows - (axis == Axis::Horizontal ? 1 : 0);
    int cols = source.cols - (axis == Axis::Vertical ? 1 : 0);
    cv::Mat_<Pixel> result(rows, cols);
    for (int y = 0; y < rows; ++y) {
        const Pixel* row = source[y];
        Pixel* output = result[y];
        if (axis == Axis::Vertical) {
            // Copy the contiguous parts on either side of this row's cut.
            const int cut = path[y];
            std::copy_n(row, cut, output);
            std::copy(row + cut + 1, row + source.cols, output + cut);
        } else {
            // Below each column's cut, take the pixel from the next row.
            const Pixel* below = source[y + 1];
            for (int x = 0; x < cols; ++x) {
                output[x] = y < path[x] ? row[x] : below[x];
            }
        }
    }
    return result;
}

void validate_path(const Seam& path, const EnergyView& view) {
    if (path.size() != static_cast<std::size_t>(view.steps())) {
        throw std::invalid_argument("Seam has the wrong length");
    }
    for (int step = 0; step < view.steps(); ++step) {
        if (path[step] < 0 || path[step] >= view.positions() ||
            (step > 0 && std::abs(path[step] - path[step - 1]) > 1)) {
            throw std::invalid_argument("Seam is out of bounds or disconnected");
        }
    }
}

} // namespace

cv::Vec3b seam_colour(Axis axis) {
    return axis == Axis::Vertical ? cv::Vec3b(0, 0, 255) : cv::Vec3b(255, 0, 0);
}

ResizeSession::ResizeSession(const Image& original, cv::Size target, Finder finder,
                             EnergyMethod method, Order order, bool track_original)
    : target_(target), finder_(std::move(finder)), method_(method), order_(order) {
    if (original.empty() || target.width < 1 || target.height < 1 || target.width > original.cols ||
        target.height > original.rows || !finder_) {
        throw std::invalid_argument("Invalid image, target size, or seam finder");
    }
    if (order != Order::VerticalFirst && order != Order::Alternate) {
        throw std::invalid_argument("Unknown seam order");
    }

    image_ = original.clone();
    if (track_original) {
        overlay_ = original.clone();
        coordinates_.create(original.rows, original.cols);
        for (int y = 0; y < original.rows; ++y) {
            for (int x = 0; x < original.cols; ++x) {
                coordinates_(y, x) = cv::Point(x, y);
            }
        }
    }
    select_axis(Axis::Vertical);
}

void ResizeSession::select_axis(Axis preferred) {
    bool vertical = image_.cols > target_.width;
    bool horizontal = image_.rows > target_.height;
    if (!vertical && !horizontal) {
        phase_ = Phase::Complete;
    } else {
        axis_ = vertical && (!horizontal || preferred == Axis::Vertical) ? Axis::Vertical
                                                                         : Axis::Horizontal;
        phase_ = Phase::NeedSeam;
    }
}

Phase ResizeSession::phase() const {
    return phase_;
}
Axis ResizeSession::axis() const {
    return axis_;
}
const Image& ResizeSession::image() const {
    return image_;
}
const Image& ResizeSession::overlay() const {
    return overlay_;
}
const Seam& ResizeSession::seam() const {
    return seam_;
}
const Stats& ResizeSession::stats() const {
    return stats_;
}

Image ResizeSession::highlighted() const {
    Image result = image_.clone();
    if (phase_ == Phase::Highlighted) {
        for (int step = 0; step < static_cast<int>(seam_.size()); ++step) {
            cv::Point point = pixel_at(axis_, step, seam_[step]);
            result(point.y, point.x) = seam_colour(axis_);
        }
    }
    return result;
}

void ResizeSession::advance() {
    if (phase_ == Phase::Complete) {
        return;
    }
    if (phase_ == Phase::NeedSeam) {
        auto start = Clock::now();
        Energy energy = compute_energy(image_, method_, axis_);
        auto energy_end = Clock::now();
        EnergyView view(energy, axis_);
        Seam candidate = finder_(view);
        auto seam_end = Clock::now();
        validate_path(candidate, view);

        double cost = 0;
        for (int step = 0; step < view.steps(); ++step) {
            cost += view(step, candidate[step]);
        }
        // Publish only a valid result, so a failed finder can be retried.
        seam_ = std::move(candidate);
        pending_cost_ = cost;
        stats_.energy_ms += milliseconds(start, energy_end);
        stats_.seam_ms += milliseconds(energy_end, seam_end);
        phase_ = Phase::Highlighted;
        return;
    }

    auto start = Clock::now();
    Image next = remove_path(image_, axis_, seam_);
    cv::Mat_<cv::Point> next_coordinates;
    if (!overlay_.empty()) {
        next_coordinates = remove_path(coordinates_, axis_, seam_);
        for (int step = 0; step < static_cast<int>(seam_.size()); ++step) {
            cv::Point point = pixel_at(axis_, step, seam_[step]);
            cv::Point original = coordinates_(point.y, point.x);
            overlay_(original.y, original.x) = seam_colour(axis_);
        }
    }
    auto end = Clock::now();

    image_ = std::move(next);
    if (!overlay_.empty()) {
        coordinates_ = std::move(next_coordinates);
    }
    stats_.remove_ms += milliseconds(start, end);
    stats_.seam_cost += pending_cost_;
    if (axis_ == Axis::Vertical) {
        ++stats_.vertical;
    } else {
        ++stats_.horizontal;
    }
    Axis preferred = order_ == Order::VerticalFirst
                         ? Axis::Vertical
                         : (axis_ == Axis::Vertical ? Axis::Horizontal : Axis::Vertical);
    seam_.clear();
    select_axis(preferred);
}

} // namespace seam

#include "image.hpp"

#include <algorithm>
#include <cmath>
#include <functional>
#include <future>
#include <stdexcept>
#include <thread>

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

namespace seam {
namespace {

void require_image(const cv::Mat& image) {
    if (image.empty() || image.rows <= 0 || image.cols <= 0) {
        throw std::invalid_argument("Image must have positive dimensions");
    }
}

// Read from the full image across strip edges; clamp only at the image border.
void sobel_rows(const Image& image, Energy& energy, int first, int last) {
    for (int y = first; y < last; ++y) {
        int top = std::max(0, y - 1);
        int bottom = std::min(image.rows - 1, y + 1);
        for (int x = 0; x < image.cols; ++x) {
            int left = std::max(0, x - 1);
            int right = std::min(image.cols - 1, x + 1);
            double sum = 0;
            for (int c = 0; c < 3; ++c) {
                int gx = image(top, right)[c] + 2 * image(y, right)[c] + image(bottom, right)[c] -
                         image(top, left)[c] - 2 * image(y, left)[c] - image(bottom, left)[c];
                int gy = image(bottom, left)[c] + 2 * image(bottom, x)[c] +
                         image(bottom, right)[c] - image(top, left)[c] - 2 * image(top, x)[c] -
                         image(top, right)[c];
                sum += std::abs(gx) + std::abs(gy);
            }
            energy(y, x) = sum;
        }
    }
}

} // namespace

cv::Point pixel_at(Axis axis, int step, int position) {
    return axis == Axis::Vertical ? cv::Point(position, step) : cv::Point(step, position);
}

EnergyView::EnergyView(const Energy& energy, Axis axis) : energy_(energy), axis_(axis) {
    require_image(energy);
}

int EnergyView::steps() const {
    return axis_ == Axis::Vertical ? energy_.rows : energy_.cols;
}

int EnergyView::positions() const {
    return axis_ == Axis::Vertical ? energy_.cols : energy_.rows;
}

double EnergyView::operator()(int step, int position) const {
    cv::Point point = pixel_at(axis_, step, position);
    return energy_(point.y, point.x);
}

Image read_image(const std::filesystem::path& path) {
    Image image = cv::imread(path.string(), cv::IMREAD_COLOR);
    if (image.empty()) {
        throw std::runtime_error("Could not read image: " + path.string());
    }
    return image;
}

void save_image(const std::filesystem::path& path, const cv::Mat& image) {
    require_image(image);
    if (!path.parent_path().empty()) {
        std::filesystem::create_directories(path.parent_path());
    }

    cv::Mat bytes;
    image.convertTo(bytes, CV_8U);
    if (!cv::imwrite(path.string(), bytes)) {
        throw std::runtime_error("Could not write image: " + path.string());
    }
}

Image downscale_to_width(const Image& image, int max_width) {
    require_image(image);
    if (max_width <= 0) {
        throw std::invalid_argument("Maximum width must be positive");
    }
    if (image.cols <= max_width) {
        return image;
    }

    int height = std::max(1, static_cast<int>(std::nearbyint(static_cast<double>(image.rows) *
                                                             max_width / image.cols)));
    Image result;
    cv::resize(image, result, cv::Size(max_width, height), 0, 0, cv::INTER_AREA);
    return result;
}

Energy compute_energy(const Image& image, EnergyMethod method, Axis axis, int workers) {
    require_image(image);
    Energy energy(image.rows, image.cols);
    if (method == EnergyMethod::Sobel || method == EnergyMethod::SobelStrips) {
        const int available = static_cast<int>(std::max(1u, std::thread::hardware_concurrency()));
        int count = method == EnergyMethod::Sobel
                        ? 1
                        : std::clamp(workers, 1, std::min(available, image.rows));
        if (count == 1) {
            sobel_rows(image, energy, 0, image.rows);
        } else {
            std::vector<std::future<void>> tasks;
            for (int i = 0; i < count; ++i) {
                int first = static_cast<int>(static_cast<long long>(image.rows) * i / count);
                int last = static_cast<int>(static_cast<long long>(image.rows) * (i + 1) / count);
                tasks.push_back(std::async(std::launch::async, sobel_rows, std::cref(image),
                                           std::ref(energy), first, last));
            }
            for (auto& task : tasks) {
                task.get();
            }
        }
        return energy;
    }

    if (method != EnergyMethod::Gradient && method != EnergyMethod::Forward) {
        throw std::invalid_argument("Unknown energy method");
    }
    for (int y = 0; y < image.rows; ++y) {
        for (int x = 0; x < image.cols; ++x) {
            int left = std::max(0, x - 1);
            int right = std::min(image.cols - 1, x + 1);
            int top = std::max(0, y - 1);
            int bottom = std::min(image.rows - 1, y + 1);
            double sum = 0;
            for (int c = 0; c < 3; ++c) {
                if (method == EnergyMethod::Forward) {
                    // Compare the pixels that would become neighbours after removal.
                    sum += axis == Axis::Vertical
                               ? std::abs(int(image(y, right)[c]) - int(image(y, left)[c]))
                               : std::abs(int(image(bottom, x)[c]) - int(image(top, x)[c]));
                } else {
                    // At a border the clamped neighbour gives a one-sided difference.
                    double dx = int(image(y, right)[c]) - int(image(y, left)[c]);
                    double dy = int(image(bottom, x)[c]) - int(image(top, x)[c]);
                    if (x > 0 && x + 1 < image.cols) {
                        dx *= 0.5;
                    }
                    if (y > 0 && y + 1 < image.rows) {
                        dy *= 0.5;
                    }
                    sum += std::abs(dx) + std::abs(dy);
                }
            }
            energy(y, x) = sum;
        }
    }
    return energy;
}

cv::Mat1b energy_image(const Energy& energy) {
    require_image(energy);
    double minimum, maximum;
    cv::minMaxLoc(energy, &minimum, &maximum);
    cv::Mat1b result(energy.rows, energy.cols);
    for (int y = 0; y < energy.rows; ++y) {
        for (int x = 0; x < energy.cols; ++x) {
            result(y, x) = maximum > minimum
                               ? static_cast<unsigned char>((energy(y, x) - minimum) * 255.0 /
                                                            (maximum - minimum))
                               : 0;
        }
    }
    return result;
}

} // namespace seam

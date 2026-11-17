#pragma once

#include <filesystem>
#include <vector>

#include <opencv2/core.hpp>

namespace seam {

using Image = cv::Mat3b;
using Energy = cv::Mat1d;
using Seam = std::vector<int>;

enum class Axis { Vertical, Horizontal };
enum class EnergyMethod { Sobel, Gradient, Forward, SobelStrips };

// A step follows the seam; a position is across it. Points use (x, y).
cv::Point pixel_at(Axis axis, int step, int position);

// Interpret the same energy map in the direction a seam travels.
class EnergyView {
public:
    EnergyView(const Energy& energy, Axis axis);
    int steps() const;
    int positions() const;
    double operator()(int step, int position) const;

private:
    Energy energy_;
    Axis axis_;
};

Image read_image(const std::filesystem::path& path);
void save_image(const std::filesystem::path& path, const cv::Mat& image);
Image downscale_to_width(const Image& image, int max_width);
Energy compute_energy(const Image& image, EnergyMethod method = EnergyMethod::Sobel,
                      Axis axis = Axis::Vertical, int workers = 4);
cv::Mat1b energy_image(const Energy& energy);

} // namespace seam

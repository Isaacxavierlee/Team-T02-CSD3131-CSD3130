#pragma once

#include "seam/image.hpp"

#include <functional>

namespace seam {

using Finder = std::function<Seam(const EnergyView&)>;
enum class Order { VerticalFirst, Alternate };
enum class Phase { NeedSeam, Highlighted, Complete };

struct Stats {
    int vertical = 0, horizontal = 0;
    double energy_ms = 0, seam_ms = 0, remove_ms = 0, seam_cost = 0;
};

cv::Vec3b seam_colour(Axis axis);

class ResizeSession {
public:
    ResizeSession(const Image& original, cv::Size target, Finder finder,
                  EnergyMethod method = EnergyMethod::Sobel, Order order = Order::VerticalFirst,
                  bool track_original = false);

    Phase phase() const;
    Axis axis() const;
    const Image& image() const;
    const Image& overlay() const;
    const Seam& seam() const;
    Image highlighted() const;
    const Stats& stats() const;
    void advance();

private:
    Image image_;
    Image overlay_;
    cv::Mat_<cv::Point> coordinates_;
    cv::Size target_;
    Finder finder_;
    EnergyMethod method_;
    Order order_;
    Phase phase_ = Phase::NeedSeam;
    Axis axis_ = Axis::Vertical;
    Seam seam_;
    Stats stats_;
    double pending_cost_ = 0;

    void select_axis(Axis preferred);
};

} // namespace seam

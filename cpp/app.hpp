#pragma once

#include "seam/resize.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace seam_app {

struct Options {
    std::filesystem::path image = "data/demo.png", out = "output/result", frames, video;
    std::string algo = "dp";
    seam::EnergyMethod energy = seam::EnergyMethod::Sobel;
    seam::Order order = seam::Order::VerticalFirst;
    int width = -1, height = -1, remove_cols = -1, remove_rows = -1, max_width = 0;
    bool interactive = false, smoke = false;
};

struct Algorithm {
    std::string name;
    seam::Finder run;
    bool gui;
};

struct EnergyChoice {
    std::string name;
    seam::EnergyMethod method;
};

const std::vector<Algorithm>& algorithms();
const std::vector<EnergyChoice>& energies();
void batch(const Options& options, const seam::Image& source, cv::Size target);
seam::Finder finder(const std::string& name);
void save_outputs(const Options& options, const seam::Image& source,
                  const seam::ResizeSession& session);
int studio(const Options& options, const seam::Image& source, cv::Size target);
} // namespace seam_app

#include "check.hpp"
#include "image.hpp"

#include <opencv2/core.hpp>

#include <filesystem>

using namespace seam;
using test::check;

namespace {

void check_filters() {
    // A one-row, one-channel ramp makes border and central differences visible.
    Image ramp(1, 3, cv::Vec3b(0, 0, 0));
    ramp(0, 1)[1] = 10;
    ramp(0, 2)[1] = 30;

    Energy sobel = compute_energy(ramp);
    check(sobel(0, 0) == 40 && sobel(0, 1) == 120 && sobel(0, 2) == 80, "Sobel at borders");
    Energy gradient = compute_energy(ramp, EnergyMethod::Gradient);
    check(gradient(0, 0) == 10 && gradient(0, 1) == 15 && gradient(0, 2) == 20,
          "one-sided and central differences");
    Energy forward = compute_energy(ramp, EnergyMethod::Forward);
    check(forward(0, 0) == 10 && forward(0, 1) == 30 && forward(0, 2) == 20, "forward cost");
    check(cv::countNonZero(compute_energy(ramp, EnergyMethod::Forward, Axis::Horizontal)) == 0,
          "a single row has no new vertical edge");

    Image flat(5, 7, cv::Vec3b(231, 8, 100));
    for (auto method : {EnergyMethod::Sobel, EnergyMethod::Gradient, EnergyMethod::Forward,
                        EnergyMethod::SobelStrips}) {
        check(cv::countNonZero(compute_energy(flat, method)) == 0, "flat image has zero energy");
    }
    check(cv::countNonZero(energy_image(compute_energy(flat))) == 0, "flat energy preview");
    test::rejects([] { compute_energy(Image{}); }, "empty image was accepted");
}

void check_axes_and_strips() {
    // Fixed random pixels exercise strip boundaries with reproducible data.
    Image photo(13, 9);
    cv::RNG(42).fill(photo, cv::RNG::UNIFORM, 0, 256);
    Energy serial = compute_energy(photo);
    for (int workers : {1, 2, 5, 20}) {
        Energy parallel = compute_energy(photo, EnergyMethod::SobelStrips, Axis::Vertical, workers);
        check(cv::norm(serial, parallel, cv::NORM_INF) == 0, "Sobel disagrees across strip edges");
    }

    EnergyView across(serial, Axis::Horizontal);
    check(across.steps() == photo.cols && across.positions() == photo.rows,
          "horizontal dimensions");
    check(across(7, 4) == serial(4, 7), "horizontal energy uses swapped coordinates");
    check(pixel_at(Axis::Vertical, 7, 4) == cv::Point(4, 7), "vertical image coordinate");
    check(pixel_at(Axis::Horizontal, 7, 4) == cv::Point(7, 4), "horizontal image coordinate");
}

void check_files(const std::filesystem::path& input) {
    Image photo = read_image(input);
    Image preview = downscale_to_width(photo, 32);
    check(preview.cols == 32 && preview.rows > 0, "width-limited evaluation preview");
    check(downscale_to_width(preview, 64).size() == preview.size(), "preview enlarged the input");

    const auto output = std::filesystem::current_path() / "test-image-output" / "roundtrip.png";
    save_image(output, preview);
    check(cv::norm(preview, read_image(output), cv::NORM_INF) == 0, "lossless PNG round trip");
}

} // namespace

int main(int argc, char** argv) {
    return test::run([&] {
        check(argc == 2, "pass the path of a JPEG fixture");
        check_filters();
        check_axes_and_strips();
        check_files(argv[1]);
    });
}

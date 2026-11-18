#include "check.hpp"
#include "dp.hpp"
#include "resize.hpp"

#include <array>
#include <vector>

using namespace seam;
using test::check;

namespace {

Image labelled_image() {
    // The first channel identifies each original pixel, even after several cuts.
    Image parent(7, 10);
    Image roi = parent(cv::Rect(1, 1, 7, 5));
    for (int y = 0; y < roi.rows; ++y) {
        for (int x = 0; x < roi.cols; ++x) {
            int id = y * roi.cols + x;
            roi(y, x) = cv::Vec3b(id, (id * 17) % 256, (id * 23) % 256);
        }
    }
    return roi;
}

void replay(cv::Size target, Order order, EnergyMethod method) {
    Image source = labelled_image();
    Image expected = source.clone();
    Image original = source.clone();
    Image overlay = source.clone();
    ResizeSession session(source, target, find_dp, method, order, true);
    int columns = 0, rows = 0;
    double removed_energy = 0;

    while (session.phase() != Phase::Complete) {
        check(session.phase() == Phase::NeedSeam, "step started in the wrong phase");
        bool expect_vertical =
            expected.cols > target.width &&
            (expected.rows == target.height || order == Order::VerticalFirst || columns <= rows);
        Axis axis = expect_vertical ? Axis::Vertical : Axis::Horizontal;
        check(session.axis() == axis, "wrong removal order");

        Energy fresh = compute_energy(expected, method, axis);
        EnergyView view(fresh, axis);
        Seam path = find_dp(view);
        session.advance();
        check(session.phase() == Phase::Highlighted && session.seam() == path,
              "seam does not match DP on freshly computed energy");
        check(cv::norm(expected, session.image(), cv::NORM_INF) == 0, "highlight removed a pixel");

        Image marked = expected.clone();
        for (int step = 0; step < view.steps(); ++step) {
            cv::Point point = pixel_at(axis, step, path[step]);
            int id = expected(point)[0];
            overlay(id / source.cols, id % source.cols) = seam_colour(axis);
            marked(point) = seam_colour(axis);
            removed_energy += view(step, path[step]);
        }
        check(cv::norm(marked, session.highlighted(), cv::NORM_INF) == 0, "wrong marked pixels");

        // Replay removal independently by collecting survivors, transposing for rows.
        Image work;
        if (expect_vertical) {
            work = expected;
            ++columns;
        } else {
            cv::transpose(expected, work);
            ++rows;
        }
        Image kept(work.rows, work.cols - 1);
        for (int y = 0; y < work.rows; ++y) {
            int out = 0;
            for (int x = 0; x < work.cols; ++x) {
                if (x != path[y]) {
                    kept(y, out++) = work(y, x);
                }
            }
        }
        if (expect_vertical) {
            expected = kept;
        } else {
            cv::transpose(kept, expected);
        }

        session.advance();
        check(cv::norm(expected, session.image(), cv::NORM_INF) == 0, "wrong surviving pixels");
        check(cv::norm(overlay, session.overlay(), cv::NORM_INF) == 0,
              "wrong original coordinates");
    }

    session.advance();
    check(session.image().size() == target && session.stats().vertical == columns &&
              session.stats().horizontal == rows && session.stats().seam_cost == removed_energy,
          "finished with the wrong size, count or energy");
    check(cv::norm(source, original, cv::NORM_INF) == 0, "input pixels were modified");
}

void check_invalid_finders() {
    Image source = labelled_image();
    test::rejects([&] { ResizeSession session(source, {8, 5}, find_dp); }, "enlargement accepted");
    test::rejects([&] { ResizeSession session(source, {0, 1}, find_dp); }, "empty target accepted");
    test::rejects([&] { ResizeSession session(source, {1, 1}, Finder{}); },
                  "missing finder accepted");

    std::vector<Seam> answers{{0}, {-1, 0, 0, 0, 0}, {0, 1, 7, 1, 0}, {0, 2, 1, 0, 0}};
    std::size_t calls = 0;
    ResizeSession session(source, {6, 5}, [&](const EnergyView& view) {
        return calls < answers.size() ? answers[calls++] : find_dp(view);
    });
    for (std::size_t i = 0; i < answers.size(); ++i) {
        test::rejects([&] { session.advance(); }, "invalid seam accepted");
        check(session.phase() == Phase::NeedSeam && session.stats().vertical == 0 &&
                  session.overlay().empty() && cv::norm(source, session.image(), cv::NORM_INF) == 0,
              "failed seam changed the session");
    }
    session.advance();
    session.advance();
    check(session.phase() == Phase::Complete, "valid retry failed");
}

} // namespace

int main() {
    return test::run([] {
        for (auto method : {EnergyMethod::Sobel, EnergyMethod::Gradient, EnergyMethod::Forward,
                            EnergyMethod::SobelStrips}) {
            for (Order order : {Order::VerticalFirst, Order::Alternate}) {
                for (cv::Size target : {cv::Size(7, 5), {7, 1}, {1, 5}, {3, 2}, {1, 1}}) {
                    replay(target, order, method);
                }
            }
        }
        check_invalid_finders();
    });
}

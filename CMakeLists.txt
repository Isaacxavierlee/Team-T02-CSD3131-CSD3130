cmake_minimum_required(VERSION 3.16)
project(SeamCarving VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenCV 4 REQUIRED COMPONENTS core imgproc imgcodecs highgui videoio)
find_package(Threads REQUIRED)
include(CTest)

add_library(seam_image src/image.cpp)
target_include_directories(seam_image PUBLIC include ${OpenCV_INCLUDE_DIRS})
target_link_libraries(seam_image PUBLIC opencv_core opencv_imgcodecs opencv_imgproc Threads::Threads)

add_library(seam_dp src/dp.cpp)
target_link_libraries(seam_dp PUBLIC seam_image)

add_library(seam_greedy src/greedy.cpp)
target_link_libraries(seam_greedy PUBLIC seam_image)

add_library(seam_graphcut src/graphcut.cpp)
target_link_libraries(seam_graphcut PUBLIC seam_image)

add_library(seam_resize src/resize.cpp)
target_link_libraries(seam_resize PUBLIC seam_image)

add_executable(seam-carve src/app.cpp src/studio.cpp)
target_link_libraries(seam-carve PRIVATE seam_resize seam_dp seam_greedy seam_graphcut opencv_highgui opencv_videoio)

add_executable(seam-profile src/profile.cpp)
target_link_libraries(seam-profile PRIVATE seam_dp)

add_executable(seam-evaluate src/evaluate.cpp)
target_link_libraries(seam-evaluate PRIVATE seam_resize seam_dp seam_greedy seam_graphcut)

if(BUILD_TESTING)
    add_executable(test_resize tests/test_resize.cpp)
    target_link_libraries(test_resize PRIVATE seam_resize seam_dp)
    add_test(NAME resize_playback COMMAND test_resize)

    add_test(NAME evaluate_photo COMMAND seam-evaluate --max-width 24 --cols 2 --rows 1
        --repeats 1 --out "${PROJECT_BINARY_DIR}/test-evaluate" "${PROJECT_SOURCE_DIR}/data/02_landscape.jpg")

    add_executable(test_dp tests/test_dp.cpp)
    target_link_libraries(test_dp PRIVATE seam_dp)
    add_test(NAME dp_optimality COMMAND test_dp)

    add_executable(test_graphcut tests/test_graphcut.cpp)
    target_link_libraries(test_graphcut PRIVATE seam_graphcut)
    add_test(NAME graphcut_optimality COMMAND test_graphcut)

    add_executable(test_image tests/test_image.cpp)
    target_link_libraries(test_image PRIVATE seam_image)
    add_test(NAME image_energy COMMAND test_image "${PROJECT_SOURCE_DIR}/data/01_landmark.jpg")

    add_executable(test_greedy tests/test_greedy.cpp)
    target_link_libraries(test_greedy PRIVATE seam_greedy)
    add_test(NAME greedy_choices COMMAND test_greedy)

    add_test(NAME command_line_resize COMMAND seam-carve "${PROJECT_SOURCE_DIR}/data/demo.png"
        --algo greedy --order alternate --max-width 44 --remove-cols 3 --remove-rows 2
        --frames-dir "${PROJECT_BINARY_DIR}/cli-frames" --out "${PROJECT_BINARY_DIR}/cli/result")
    add_test(NAME conflicting_targets COMMAND seam-carve "${PROJECT_SOURCE_DIR}/data/demo.png"
        --width 4 --remove-cols 5)
    set_tests_properties(conflicting_targets PROPERTIES WILL_FAIL TRUE)
endif()

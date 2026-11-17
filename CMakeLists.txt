cmake_minimum_required(VERSION 3.16)
project(SeamCarving VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenCV 4 REQUIRED COMPONENTS core imgproc imgcodecs highgui videoio)
find_package(Threads REQUIRED)
include(CTest)

add_library(seam_image cpp/image.cpp)
target_include_directories(seam_image PUBLIC include ${OpenCV_INCLUDE_DIRS})
target_link_libraries(seam_image PUBLIC ${OpenCV_LIBS} Threads::Threads)

add_library(seam_dp cpp/dp.cpp)
target_link_libraries(seam_dp PUBLIC seam_image)

add_library(seam_resize cpp/resize.cpp)
target_link_libraries(seam_resize PUBLIC seam_image)

if(BUILD_TESTING)
    add_executable(test_resize tests/cpp/test_resize.cpp)
    target_link_libraries(test_resize PRIVATE seam_resize seam_dp)
    add_test(NAME resize_playback COMMAND test_resize)

    add_executable(test_dp tests/cpp/test_dp.cpp)
    target_link_libraries(test_dp PRIVATE seam_dp)
    add_test(NAME dp_optimality COMMAND test_dp)
endif()

if(BUILD_TESTING)
    add_executable(test_image tests/cpp/test_image.cpp)
    target_link_libraries(test_image PRIVATE seam_image)
    add_test(NAME image_energy COMMAND test_image "${PROJECT_SOURCE_DIR}/data/01_landmark.jpg")
endif()

add_library(seam_greedy cpp/greedy.cpp)
target_link_libraries(seam_greedy PUBLIC seam_image)

add_executable(seam-carve cpp/app.cpp cpp/studio.cpp)
target_link_libraries(seam-carve PRIVATE seam_resize seam_dp seam_greedy opencv_highgui opencv_videoio)

if(BUILD_TESTING)
    add_executable(test_greedy tests/cpp/test_greedy.cpp)
    target_link_libraries(test_greedy PRIVATE seam_greedy)
    add_test(NAME greedy_choices COMMAND test_greedy)

    add_test(NAME command_line_resize COMMAND seam-carve "${PROJECT_SOURCE_DIR}/data/demo.png"
        --algo greedy --order alternate --max-width 44 --remove-cols 3 --remove-rows 2
        --frames-dir "${PROJECT_BINARY_DIR}/cli-frames" --out "${PROJECT_BINARY_DIR}/cli/result")
    add_test(NAME conflicting_targets COMMAND seam-carve "${PROJECT_SOURCE_DIR}/data/demo.png"
        --width 4 --remove-cols 5)
    set_tests_properties(conflicting_targets PROPERTIES WILL_FAIL TRUE)
endif()

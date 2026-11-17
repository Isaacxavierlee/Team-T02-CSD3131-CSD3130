cmake_minimum_required(VERSION 3.16)
project(SeamCarving VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenCV 4 REQUIRED COMPONENTS core imgproc imgcodecs)
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

if(BUILD_TESTING)
    add_executable(test_greedy tests/cpp/test_greedy.cpp)
    target_link_libraries(test_greedy PRIVATE seam_greedy)
    add_test(NAME greedy_choices COMMAND test_greedy)
endif()

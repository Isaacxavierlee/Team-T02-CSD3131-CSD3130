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

add_executable(seam-evaluate cpp/evaluate.cpp)
target_link_libraries(seam-evaluate PRIVATE seam_resize seam_dp seam_greedy)

option(SEAM_WITH_PYTHON_GRAPHCUT "Compare the repository's original Python graph-cut solver" OFF)
if(SEAM_WITH_PYTHON_GRAPHCUT)
    find_package(Python3 REQUIRED COMPONENTS Interpreter Development)
    set(PYBIND11_FINDPYTHON ON)
    execute_process(COMMAND "${Python3_EXECUTABLE}" -m pybind11 --cmakedir
        OUTPUT_VARIABLE pybind11_hint OUTPUT_STRIP_TRAILING_WHITESPACE)
    find_package(pybind11 CONFIG REQUIRED HINTS "${pybind11_hint}")
    execute_process(COMMAND "${Python3_EXECUTABLE}" -c
        "import sysconfig; print(sysconfig.get_path('purelib'))"
        OUTPUT_VARIABLE python_site OUTPUT_STRIP_TRAILING_WHITESPACE)
    file(TO_CMAKE_PATH "${python_site}" python_site)
    execute_process(COMMAND "${Python3_EXECUTABLE}" -c "import sys; print(sys.base_prefix)"
        OUTPUT_VARIABLE python_home OUTPUT_STRIP_TRAILING_WHITESPACE)
    file(TO_CMAKE_PATH "${python_home}" python_home)

    add_library(seam_python_baseline cpp/python_baseline.cpp)
    target_include_directories(seam_python_baseline PUBLIC cpp)
    target_link_libraries(seam_python_baseline PUBLIC seam_image pybind11::embed)
    target_compile_definitions(seam_python_baseline PRIVATE
        SEAM_LEGACY_ALGORITHMS="${PROJECT_SOURCE_DIR}/seam_carving/algorithms"
        SEAM_PYTHON_SITE="${python_site}" SEAM_PYTHON_HOME="${python_home}")
    target_compile_definitions(seam-evaluate PRIVATE SEAM_WITH_PYTHON_GRAPHCUT)
    target_link_libraries(seam-evaluate PRIVATE seam_python_baseline)
endif()

if(BUILD_TESTING)
    add_executable(test_resize tests/cpp/test_resize.cpp)
    target_link_libraries(test_resize PRIVATE seam_resize seam_dp)
    add_test(NAME resize_playback COMMAND test_resize)

    add_test(NAME evaluate_photo COMMAND seam-evaluate --max-width 24 --cols 2 --rows 1
        --repeats 1 --out "${PROJECT_BINARY_DIR}/test-evaluate" "${PROJECT_SOURCE_DIR}/data/02_landscape.jpg")
    if(SEAM_WITH_PYTHON_GRAPHCUT)
        add_executable(test_graph_baseline tests/cpp/test_graph_baseline.cpp)
        target_link_libraries(test_graph_baseline PRIVATE seam_python_baseline seam_dp seam_greedy)
        add_test(NAME compare_seam_costs COMMAND test_graph_baseline)
        add_test(NAME python_graphcut_comparison COMMAND seam-evaluate --with-graphcut
            --max-width 16 --cols 1 --rows 1 --repeats 1 --out "${PROJECT_BINARY_DIR}/test-graphcut"
            "${PROJECT_SOURCE_DIR}/data/02_landscape.jpg")
    endif()

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

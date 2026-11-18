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
target_link_libraries(seam_image PUBLIC opencv_core opencv_imgcodecs opencv_imgproc Threads::Threads)

add_library(seam_dp cpp/dp.cpp)
target_link_libraries(seam_dp PUBLIC seam_image)

add_library(seam_greedy cpp/greedy.cpp)
target_link_libraries(seam_greedy PUBLIC seam_image)

add_library(seam_graphcut cpp/graphcut.cpp)
target_link_libraries(seam_graphcut PUBLIC seam_image)

add_library(seam_resize cpp/resize.cpp)
target_link_libraries(seam_resize PUBLIC seam_image)

add_executable(seam-carve cpp/app.cpp cpp/studio.cpp)
target_link_libraries(seam-carve PRIVATE seam_resize seam_dp seam_greedy seam_graphcut opencv_highgui opencv_videoio)

add_executable(seam-profile cpp/profile.cpp)
target_link_libraries(seam-profile PRIVATE seam_dp)

add_executable(seam-evaluate cpp/evaluate.cpp)
target_link_libraries(seam-evaluate PRIVATE seam_resize seam_dp seam_greedy seam_graphcut)

option(SEAM_WITH_PYTHON_GRAPHCUT "Compare the repository's original Python graph-cut solver" OFF)
if(SEAM_WITH_PYTHON_GRAPHCUT)
    find_package(Python3 3.8 REQUIRED COMPONENTS Interpreter Development)
    set(PYBIND11_FINDPYTHON ON)
    execute_process(COMMAND "${Python3_EXECUTABLE}" -m pybind11 --cmakedir
        OUTPUT_VARIABLE pybind11_hint OUTPUT_STRIP_TRAILING_WHITESPACE)
    find_package(pybind11 CONFIG REQUIRED HINTS "${pybind11_hint}")
    foreach(package IN ITEMS numpy networkx)
        execute_process(COMMAND "${Python3_EXECUTABLE}" -c
            "from pathlib import Path; import ${package}; print(Path(${package}.__file__).resolve().parents[1])"
            OUTPUT_VARIABLE package_dir RESULT_VARIABLE import_status OUTPUT_STRIP_TRAILING_WHITESPACE)
        if(NOT import_status EQUAL 0)
            message(FATAL_ERROR "Install ${package} for ${Python3_EXECUTABLE} to enable graph-cut comparison")
        endif()
        file(TO_CMAKE_PATH "${package_dir}" ${package}_site)
    endforeach()
    execute_process(COMMAND "${Python3_EXECUTABLE}" -c "import sys; print(sys.base_prefix)"
        OUTPUT_VARIABLE python_home OUTPUT_STRIP_TRAILING_WHITESPACE)
    file(TO_CMAKE_PATH "${python_home}" python_home)

    add_library(seam_python_baseline cpp/python_baseline.cpp)
    target_include_directories(seam_python_baseline PUBLIC cpp)
    target_link_libraries(seam_python_baseline PUBLIC seam_image pybind11::embed)
    target_compile_definitions(seam_python_baseline PRIVATE
        SEAM_LEGACY_ALGORITHMS="${PROJECT_SOURCE_DIR}/seam_carving/algorithms"
        SEAM_NUMPY_SITE="${numpy_site}" SEAM_NETWORKX_SITE="${networkx_site}" SEAM_PYTHON_HOME="${python_home}")
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

    add_executable(test_image tests/cpp/test_image.cpp)
    target_link_libraries(test_image PRIVATE seam_image)
    add_test(NAME image_energy COMMAND test_image "${PROJECT_SOURCE_DIR}/data/01_landmark.jpg")

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

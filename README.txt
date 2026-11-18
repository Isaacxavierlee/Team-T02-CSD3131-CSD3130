TEAM T02 - IMAGE RESIZING BY SEAM CARVING
=========================================

The project uses C++17 and OpenCV 4 to reduce image width and height by
removing low-energy seams. It implements dynamic programming (DP), greedy
search and graph cut. The interactive viewer provides DP and greedy variants.
The command-line evaluator compares all 3 C++ methods on the same input.

The project report is Seam-Carving-Report.docx. The saved comparisons and their
measurements are described in evaluation/README.md.

1. RUN THE WINDOWS APPLICATION

The Windows CI job packages the executable, OpenCV DLLs and example images in
the SeamCarving-Windows-x64 GitHub Actions artifact. Download the artifact from
a successful run for the revision you want to use and extract it. From a
PowerShell terminal in the extracted folder, run:

   .\seam-carve.exe data/05_architecture.jpg --interactive --max-width 80 --width 72 --height 101

The width, height, algorithm and energy sliders restart the resize from the
prepared input. Space plays or pauses, N advances 1 phase, R restarts,
S saves, and Q or Escape exits. Each seam is highlighted before removal.
Red marks vertical seams and blue marks horizontal seams. The left view shows
all removed seams on the original image. The right view shows the current
image. Use --help for the command-line options.

2. BUILD FROM SOURCE ON WINDOWS

Install Visual Studio 2022 with Desktop development with C++, CMake, and the
OpenCV 4 Windows SDK. The CI workflow uses OpenCV 4.12.0:

   https://github.com/opencv/opencv/releases/tag/4.12.0

Extract the SDK to C:\, which creates C:\opencv, or adjust the paths below.
Run these commands from the repository root in PowerShell:

   $env:OpenCV_DIR = 'C:/opencv/build'
   $env:PATH = 'C:\opencv\build\x64\vc16\bin;' + $env:PATH
   cmake -S . -B build/native -G "Visual Studio 17 2022" -A x64 -DOpenCV_DIR="$env:OpenCV_DIR"
   cmake --build build/native --config Release --parallel
   ctest --test-dir build/native -C Release --output-on-failure
   .\build\native\Release\seam-carve.exe data/05_architecture.jpg --interactive --max-width 80 --width 72 --height 101

OpenCV_DIR must point to the folder containing OpenCVConfig.cmake. Its DLL
folder must be on PATH when running the executables and tests. run.bat can also
configure, build and launch seam-carve when the SDK is available.

To produce a batch result and individual highlight and removal frames:

   .\build\native\Release\seam-carve.exe data/demo.png --algo dp --max-width 80 --remove-cols 4 --remove-rows 3 --frames-dir output/steps --out output/demo

The output prefix produces the resized PNG, an overlay and an energy image.
The frames folder contains 2 images per removed seam. A --video FILE option
is available when a compatible video encoder is installed.

3. COMPARE THE ALGORITHMS

After building, evaluate the 5 photographs with the same Sobel energy map
and vertical-first removal order:

   $photos = @(Get-ChildItem data -Filter '*.jpg' | Sort-Object Name | ForEach-Object { $_.FullName })
   .\build\native\Release\seam-evaluate.exe --max-width 48 --cols 4 --rows 3 --repeats 3 --out output/eval @photos

For each image the evaluator writes the prepared input, the resized image and
seam overlay for each method, and a comparison panel. The generated file
output/eval/results.csv records dimensions, timings, total seam energy,
pixel differences and differing paths against DP. Timings depend on the
machine and build.

4. TESTS AND RESULTS

CTest runs 7 checks: resize_playback, evaluate_photo, dp_optimality,
image_energy, greedy_choices, command_line_resize and conflicting_targets.
The DP test compares both vertical and horizontal seams with exhaustive
enumeration on small maps. The greedy test checks its local choices. The
evaluator test runs all 3 methods and checks repeatability. It does not
establish graph-cut optimality.

The current graph-cut implementation uses Edmonds-Karp. Its last image
column connects directly to the sink without an energy-weighted edge, so a
lowest-energy boundary seam can be missed. The report gives a concrete
counterexample and the saved 5-photo results.

evaluation/algorithm-comparison.csv records timings and seam costs for DP,
greedy and graph cut. The 6 panels in evaluation/comparisons/ show the
resized images and seam overlays.
evaluation/dp-memory-and-sobel-threading.csv records DP time and buffer size for
2 storage modes, plus Sobel time for 1, 2, 4 and 8 workers.

Source files:

- cpp/image.cpp computes energy maps and handles images.
- cpp/dp.cpp, cpp/greedy.cpp and cpp/graphcut.cpp find seams.
- cpp/resize.cpp validates, highlights and removes seams.
- cpp/studio.cpp implements the interactive viewer.
- cpp/evaluate.cpp runs the comparison and writes its results.
- tests/cpp contains the C++ unit tests.

The .github/workflows/ci.yml workflow builds and tests on Linux and Windows.
It also uploads example outputs and a Windows executable package.

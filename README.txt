TEAM T02 - IMAGE RESIZING BY SEAM CARVING
=========================================

This guide is for the marker or any reader who wants to run the project and
understand what is being compared. The root folder also contains
scriptsreadme.txt, which is the team's presentation script.

PROJECT SCOPE

The native application uses C++17, CMake and OpenCV 4. It implements dynamic
programming (DP), greedy seam finding, energy computation, repeated removal,
live resizing and step-by-step playback. An optional C++ evaluator calls the
Python Dinic graph-cut implementation through pybind11. A separate original
Python pipeline is also present. The native graphical app offers DP and greedy
variants. Graph cut runs through the optional evaluator or Python pipeline.

1. FASTEST WAY TO TRY THE WINDOWS APPLICATION

1. Open the repository's Actions page. Choose a successful CI run on the
   desired commit and download the artifact named SeamCarving-Windows-x64.
   GitHub may require you to sign in, and artifacts expire. One verified
   successful run for commit 5f3375b is:
   https://github.com/Isaacxavierlee/Team-T02-CSD3131-CSD3130/actions/runs/44263568971
2. Extract the entire artifact. Keep seam-carve.exe, the OpenCV DLLs and data
   folder together. Open PowerShell in the extracted folder.
3. Open the architecture example with a known input and target:

   .\seam-carve.exe data/05_architecture.jpg --interactive --max-width 80 --width 72 --height 101

4. In the window, width, height, algorithm and energy sliders restart the
   resize from the original prepared input. Space plays or pauses. N advances
   one phase, R restarts, S saves, and Q or Escape exits. Red marks vertical
   seams and blue marks horizontal seams. Each seam is highlighted before
   the next phase removes it. The left view accumulates seams on the original.

The executable's default window uses data/demo.png when launched with no
arguments. A small --max-width is useful because each removal recalculates the
energy map. Press S to save the current result, overlay and energy image under
the default output/result prefix. Use --help for other command-line options.

2. BUILD THE C++ PROJECT FROM SOURCE ON WINDOWS

Install Visual Studio 2022 with Desktop development with C++, CMake, and the
OpenCV 4 development SDK. Extract OpenCV, for example to C:\opencv. In a
PowerShell terminal at the repository root, adjust these paths for your SDK:

   $env:OpenCV_DIR = 'C:/opencv/build'
   $env:PATH = 'C:\opencv\build\x64\vc16\bin;' + $env:PATH
   cmake -S . -B build/native -G "Visual Studio 17 2022" -A x64 -DOpenCV_DIR="$env:OpenCV_DIR"
   cmake --build build/native --config Release --parallel
   ctest --test-dir build/native -C Release --output-on-failure
   .\build\native\Release\seam-carve.exe data/05_architecture.jpg --interactive --max-width 80 --width 72 --height 101

Use the bin directory shipped with the OpenCV SDK if it differs from the
example. The OpenCV_DIR folder must contain OpenCVConfig.cmake. The OpenCV
DLLs must be on PATH for the executable and tests to run. run.bat is an
alternative launcher after the dependencies are available.

For a batch output with DP, including the two playback phases for each seam:

   .\build\native\Release\seam-carve.exe data/demo.png --algo dp --max-width 80 --remove-cols 4 --remove-rows 3 --frames-dir output/steps --out output/demo

The output prefix produces a resized PNG, a seam-overlay PNG and an energy
image. The steps folder contains highlighted and removed frames. A --video
FILE option is available if a compatible video encoder is installed.

3. REPRODUCE THE THREE-METHOD COMPARISON

This optional build needs the same C++ dependencies plus Python 3 with
development headers, NumPy, NetworkX and pybind11. Install the packages using
the same Python interpreter passed to CMake:

   python -m pip install numpy networkx pybind11
   cmake -S . -B build/compare -G "Visual Studio 17 2022" -A x64 -DOpenCV_DIR="$env:OpenCV_DIR" -DSEAM_WITH_PYTHON_GRAPHCUT=ON
   cmake --build build/compare --config Release --parallel
   ctest --test-dir build/compare -C Release --output-on-failure
   .\build\compare\Release\seam-evaluate.exe --with-graphcut --max-width 48 --cols 4 --rows 3 --repeats 3 --out output/eval data/05_architecture.jpg

If CMake selects the wrong Python, add
  -DPython3_EXECUTABLE=C:/path/to/the/same/python.exe
to the configure command. The evaluator writes the input, outputs, original-
coordinate seam overlays, a comparison panel and results.csv beneath output/eval.
The graph-cut comparison calls seam_carving/algorithms/graphcut.py through
cpp/python_baseline.cpp. The separate seam_carving/graphcut.py is not selected
by this bridge. The timed graph-cut pass includes conversion and transfer
through the Python bridge. Python interpreter startup is excluded.

4. READ THE RESULTS AND IMPLEMENTATION

- evaluation/README.md explains the recorded experiment and its environment.
  evaluation/width-48/ and evaluation/width-80/ contain comparison images and
  the raw CSV records. At each size five images were evaluated, with three
  timed repetitions per method after an untimed tracked pass.
- evaluation/dp-profile.csv contains the DP buffer and Sobel worker profiling.
- cpp/image.cpp handles image loading and energy maps. cpp/dp.cpp and
  cpp/greedy.cpp implement the native seam finders. cpp/resize.cpp validates
  and removes seams. cpp/studio.cpp contains the interactive window.
- seam_carving/algorithms/graphcut.py constructs the graph and solves its
  minimum cut. cpp/python_baseline.cpp connects it to the optional evaluator.
- tests/cpp/ and tests/test_all.py contain correctness checks. The default
  Windows build tests the native code. The optional comparison build adds
  graph-cut checks. The GitHub workflow contains separate Windows, native
  Linux and Python jobs in .github/workflows/ci.yml.

Interpret timing as a measurement of these implementations on the reported
machine. Greedy, DP and graph cut are fed the same Sobel energy for the saved
comparison. Graph cut is Python while the other two are C++. A low-energy
seam is not a guarantee of perceived visual quality, so inspect the images
and overlays alongside the numbers.

The assignment report and recording link are separate submission items. For a
specific submission, compare the report's descriptions and figures against
the exact commit and evaluation outputs being submitted.

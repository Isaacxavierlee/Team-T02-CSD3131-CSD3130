# Seam carving · Assignment 2

## Build the C++ implementation

Use a C++17 compiler, CMake 3.16 or later and the OpenCV 4 development package. On Ubuntu install
`libopencv-dev`. On Windows install Visual Studio's **Desktop development with C++** workload,
extract the [OpenCV Windows SDK](https://opencv.org/releases/), and pass
`-DOpenCV_DIR=C:/opencv/build` to the configure command. Add the SDK's `x64/vc16/bin` directory
to `PATH` so Windows can find its DLLs.

```sh
cmake -S . -B build/native -DCMAKE_BUILD_TYPE=Release
cmake --build build/native --config Release --parallel
ctest --test-dir build/native -C Release --output-on-failure
```

## Run the C++ application

From the repository root, double-click `run.bat` on Windows or run `bash run.sh` on Linux
or macOS. Both launchers build the application and open the demo photo. Pass an image path
and `--interactive` to open another photo. To call the executable directly, use
`build/native/Release/seam-carve.exe` with Visual Studio or `build/native/seam-carve` with
a single-configuration generator.

```sh
seam-carve data/05_architecture.jpg --interactive --max-width 240
seam-carve data/03_person.jpg --algo greedy --remove-cols 20 --remove-rows 8 --out output/portrait
seam-carve data/demo.png --algo dp --order alternate --remove-cols 5 --remove-rows 3 --frames-dir output/steps --video output/steps.mp4
```

In the window, adjust width, height, algorithm or energy to start a new resize from the input.
Space pauses or plays, N advances one phase, R restarts, S saves and Q or Escape closes the
window. Red marks vertical seams and blue marks horizontal seams. Each removal has two phases.
The first shows the chosen pixels on the current image. The second shows the surviving pixels.
The original-sized overlay retains the history as the image shrinks.

In batch mode, `--width` and `--height` give exact target dimensions. Alternatively use
`--remove-cols` or `--remove-rows` on the corresponding axis. `--max-width` prepares a
smaller input before carving. The output prefix names the resized PNG, seam overlay and energy
map. `--frames-dir` exports each highlight and removal, and `--video` encodes those phases
at a fixed display size. Run `seam-carve --help` for the method and energy choices.

The native sources use four-space indentation. One blank line separates stages such as filling
costs and tracing a seam. Comments explain choices that affect the answer, including tie order,
border sampling and coordinate tracking. The formatting settings are in `.clang-format`.
[Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html#Vertical_Whitespace) ·
[C++ Core Guidelines on comments](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#nl2-state-intent-in-comments) ·
[LLVM Coding Standards](https://llvm.org/docs/CodingStandards.html#commenting)

## Energy and seam search

The image library loads colour photos and computes energy from Sobel derivatives, finite
differences or the difference between the two pixels a removal would bring together. Sobel
replicates edge pixels and sums the absolute horizontal and vertical response over all three
colour channels. The strip variant reads the same neighbouring rows while workers write to
separate regions of the output. Both use Θ(height × width) work and space for the map.

### Dynamic programming

Treat one row as a step for a vertical seam and one column as a step for a horizontal seam.
Let `D(s,p)` be the minimum energy of a connected seam ending at position `p` in step `s`.
The first step costs its own pixel energy. At each later pixel, add its energy to the smallest
of the up to three reachable predecessor costs. Any cheaper route to that predecessor could
replace the prefix of the seam, which is why keeping its minimum suffices. Start at the cheapest
final position and follow the recorded predecessor offsets backwards. Ties use the lowest
position at each decision.

`find_dp` visits each pixel once. For an H × W image its time is Θ(HW). Reconstruction needs
one signed byte per pixel, plus two rows of doubles (two columns for a horizontal seam).
`find_dp_full` retains every cumulative cost for inspection. Both versions return the same
path. The `dp_optimality` test enumerates every legal path on small maps in both directions.

### Greedy seam search

`find_greedy` starts at the first minimum in the entry row, then takes the cheapest reachable
pixel at each step. For a vertical seam it scans W starting pixels and at most three pixels in
each of the next H − 1 rows. Search takes Θ(W + H) time and constant workspace beyond the path.
The horizontal search uses columns as steps. Energy calculation and removal still visit the
whole image on each resize step.

A local choice can force an expensive later one. The `greedy_choices` test has a zero-cost
start that leads to three pixels costing 50 each. Starting two columns away costs 1 per step,
for a total of 4. The optional three-step lookahead finds that route on this fixture. It has
a higher constant search cost and can still miss a minimum beyond its horizon.

## Original Python pipeline

Isaac's original implementation can be run directly for comparison.
```bash
pip install -r requirements.txt
python scripts/make_demo_image.py
python main.py data/demo.png --algo dp --remove-cols 40 --remove-rows 20          # algos: dp, greedy, graphcut (+ variants)
python main.py data/demo.png --algo graphcut --max-width 60 --remove-cols 8      # graph cut is slow: small images
python main.py data/demo.png --algo dp --remove-cols 30 --frames-dir output/frames --video output/seams.mp4   # bonus 7a
python evaluate.py data/*.png --cols 10 20 --rows 5 --max-width 80               # comparison tables + images
python scripts/benchmark_scaling.py                                              # time vs size (Spec 5b)
python scripts/experiment_threads.py                                             # D&C / threads / memory (Spec 2b, 5d)
python scripts/greedy_failure_case.py                                            # greedy vs DP counter-example
pytest -q
```

### Python files
| File | Owner | Spec |
|---|---|---|
| `seam_carving/io_utils.py`, `energy.py`, `visualize.py`, `main.py`, `gui.py`, `run.bat` | Isaac | 1a, 2b, 7a, 7b |
| `seam_carving/algorithms/dp.py` | Dalton | 1b, 2a, 5b, 5d |
| `seam_carving/algorithms/greedy.py` | Rui | 3 |
| `seam_carving/algorithms/graphcut.py` | Dexter | 4 |
| `seam_carving/carver.py`, `evaluate.py`, `tests/` | Fadzly | 1c, 3, 4 comparison |

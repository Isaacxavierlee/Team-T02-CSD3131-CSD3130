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

Run `seam-profile data/05_architecture.jpg` from the repository using the executable in your
build directory. Its CSV compares five runs per configuration on progressively wider copies of
the same photo. The Sobel runs request one, two, four or eight workers. The DP runs use one
precomputed energy map per size. `dp_work_buffer_bytes` counts the cost and parent arrays. The
input map, returned path and allocator overhead are outside this count.

The [recorded profile](evaluation/dp-profile.csv) used `05_architecture.jpg`, MSVC 19.44
Release, OpenCV 4.12 and a Core i7-13700HX. At 512 × 682, rolling DP used 357,376 bytes of
working buffers and full DP used 3,142,656. Four Sobel workers took 1.11 ms per map at this
size and 0.09 ms at 64 × 85. The serial times were 3.46 ms and 0.05 ms respectively.

## Original Python pipeline

**Quick start (Windows):** double-click `run.bat`  (Mac/Linux: `./run.sh`). It updates the code, sets up Python libraries and opens the app.

Command line:
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
| `docs/` | Ranvitha | 2, 5, 8, 9 |

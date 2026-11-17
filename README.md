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

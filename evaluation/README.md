# Five-photo resize comparison

Each of the five repository JPEGs was prepared at two widths with its aspect ratio preserved.
At width 48 we removed four columns and three rows. At width 80 we removed eight columns and
six rows. All methods used the same decoded input, Sobel energy and vertical-first order.
Each case has one untimed pass for the pictures and three timed passes, for 90 timing records.
The raw measurements are in [width 48](width-48/results.csv) and [width 80](width-80/results.csv).

## Results at width 80

Times are median wall-clock milliseconds for the complete resize. The last column is the sum
of the Sobel energies removed by native DP and greedy, respectively.

| Photo | Resize | DP C++ (ms) | Greedy C++ (ms) | Graph cut Python (ms) | DP / greedy energy |
|---|---|---:|---:|---:|---:|
| [Landmark](width-80/01_landmark.png) | 80 × 147 → 72 × 141 | 1.489 | 1.072 | 20,926.630 | 134,346 / 203,852 |
| [Landscape](width-80/02_landscape.png) | 80 × 64 → 72 × 58 | 0.666 | 0.497 | 4,094.785 | 87,824 / 103,408 |
| [Portrait](width-80/03_person.png) | 80 × 120 → 72 × 114 | 1.369 | 1.042 | 17,145.767 | 127,898 / 254,444 |
| [Texture](width-80/04_texture.png) | 80 × 53 → 72 × 47 | 0.582 | 0.569 | 3,783.167 | 242,300 / 347,682 |
| [Architecture](width-80/05_architecture.png) | 80 × 107 → 72 × 101 | 1.048 | 0.758 | 8,582.903 | 86,526 / 321,096 |

DP and graph cut chose identical paths and produced identical pixels in all ten photo/size
cases. Greedy removed more energy in every case. The architecture overlay shows several greedy
horizontal seams crossing the window grid. DP's horizontal seams run mostly through the sky.
In the portrait, greedy crosses the eyes while DP's horizontal seams stay above the face.
The landscape outputs are closer, with a mean absolute channel difference of 1.76 on a 0–255 scale.

## Reproduce the measurements

Use the optional graph-cut comparison build in the [project instructions](../README.txt).
From the repository root, run these commands in PowerShell.

```powershell
$photos = Get-ChildItem data -Filter '*.jpg' | Sort-Object Name | ForEach-Object { "data/$($_.Name)" }
$evaluator = '.\build\compare\Release\seam-evaluate.exe'
& $evaluator --with-graphcut --max-width 48 --cols 4 --rows 3 --repeats 3 --out output/compare-48 @photos
& $evaluator --with-graphcut --max-width 80 --cols 8 --rows 6 --repeats 3 --out output/compare-80 @photos
```

The recorded build used Windows x64, a Core i7-13700HX, MSVC 19.44 Release, OpenCV 4.12,
Python 3.9.13 and NumPy 2.0.2. DP and greedy run in C++. The comparison calls the repository's
existing Python Dinic solver from the same C++ resize loop. Its search time includes transfer
of the energy map and returned seam across the language boundary. Interpreter startup, reading
images, preparing previews, painting overlays and writing files occur outside timed passes.

The CSV stage times measure energy calculation, seam search and pixel removal. Total time also
includes session setup, validation and accounting. The evaluator checks that each timed pass
reproduces its recorded image and cost. `different_paths_vs_dp` counts removal steps where
the two paths differ in the coordinates of that step's current image. The overlays instead
mark all removals on the original preview. Red indicates a vertical seam and blue a horizontal seam.

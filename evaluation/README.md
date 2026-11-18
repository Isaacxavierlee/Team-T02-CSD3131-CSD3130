# C++ seam-carving comparison

The [algorithm comparison](algorithm-comparison.csv) covers 5 photographs
and the [synthetic demo](comparisons/demo.png) at preview widths of 48 and 80
pixels. Area resampling preserved each image's aspect ratio. At width 48,
each method removed 4 columns followed by 3 rows. At width 80, each removed
8 columns followed by 6 rows. All 3 methods used Sobel energy.

For each method and image, the evaluator ran 1 untimed pass to record images
and paths, followed by 5 timed passes. The CSV contains 180 timed records.
The [comparison panels](comparisons/) show the width-48 input, its Sobel
energy map, and each method's resized image and seam overlay.

## Width 48 results

Times are the median complete-resize time in milliseconds. Energy is the sum
of the 7 seams removed from their changing energy maps.

| Photo | Input | Target | DP (ms) | Greedy (ms) | Graph cut (ms) | DP energy | Greedy energy | Graph-cut energy |
|---|---|---|---:|---:|---:|---:|---:|---:|
| [Landmark](comparisons/01_landmark.png) | 48 x 88 | 44 x 85 | 0.396 | 0.200 | 423.207 | 48,762 | 66,132 | 48,762 |
| [Landscape](comparisons/02_landscape.png) | 48 x 38 | 44 x 35 | 0.125 | 0.090 | 44.415 | 40,976 | 50,214 | 40,976 |
| [Portrait](comparisons/03_person.png) | 48 x 72 | 44 x 69 | 0.209 | 0.157 | 1313.303 | 51,820 | 98,924 | 51,820 |
| [Texture](comparisons/04_texture.png) | 48 x 32 | 44 x 29 | 0.487 | 0.460 | 338.213 | 79,098 | 111,658 | 79,098 |
| [Architecture](comparisons/05_architecture.png) | 48 x 64 | 44 x 61 | 1.010 | 0.707 | 593.692 | 33,582 | 95,484 | 33,582 |

## Width 80 results

These runs remove 14 seams per image. The change in time reflects both the
larger inputs and the larger number of removals.

| Photo | Input | Target | DP (ms) | Greedy (ms) | Graph cut (ms) | DP energy | Greedy energy | Graph-cut energy |
|---|---|---|---:|---:|---:|---:|---:|---:|
| Landmark | 80 x 147 | 72 x 141 | 1.822 | 1.040 | 28080.634 | 134,346 | 203,852 | 134,346 |
| Landscape | 80 x 64 | 72 x 58 | 3.175 | 2.226 | 3894.150 | 87,824 | 103,408 | 87,824 |
| Portrait | 80 x 120 | 72 x 114 | 5.829 | 4.058 | 4477.642 | 127,898 | 254,444 | 127,898 |
| Texture | 80 x 53 | 72 x 47 | 0.558 | 0.425 | 1212.864 | 242,300 | 347,682 | 242,300 |
| Architecture | 80 x 107 | 72 x 101 | 1.252 | 0.952 | 1535.939 | 86,526 | 321,096 | 86,526 |

For these 5 photos at both widths, greedy removed more total energy than DP.
Graph cut and DP selected the same paths and produced identical pixels for
every recorded resize. Other inputs can have different equal-cost seams.

The comparison ran on an Intel Core i7-14650HX with 24 logical processors,
Windows NT 10.0.26200, MSVC 19.44.35228, OpenCV 4.12.0 and a Release build.
Background load and processor frequency were not controlled, so these times
describe this run rather than universal rankings.

The evaluator reports energy calculation, seam search, removal and total time.
The timed passes exclude image loading, preview preparation, overlay recording
and file output. Total time also includes session setup and validation. The
untimed pass records original-coordinate overlays and paths. Red marks
vertical seams and blue marks horizontal seams. `different_paths_vs_dp`
counts steps whose path differs from DP in that step's current image.

The [DP memory and Sobel threading measurements](dp-memory-and-sobel-threading.csv)
give the mean time over 5 runs for each DP storage mode and Sobel worker
count. The byte counts cover the DP cost and parent buffers. This separate
earlier profile did not record its hardware or compiler.

## Run the comparison

Build the project using the [project instructions](../README.txt), then run:

```powershell
$photos = @(Get-ChildItem data -Filter '*.jpg' | Sort-Object Name | ForEach-Object { $_.FullName })
.\build\native\Release\seam-evaluate.exe --max-width 48 --cols 4 --rows 3 --repeats 5 --out output/compare-48 @photos data/demo.png
.\build\native\Release\seam-evaluate.exe --max-width 80 --cols 8 --rows 6 --repeats 5 --out output/compare-80 @photos data/demo.png
```

Each command writes a `results.csv` and a folder per input with its PNGs
and comparison panel. Timing values will vary.

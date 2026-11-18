# C++ seam-carving comparison

The [C++ results CSV](graphcut_results/eval/results.csv) contains 5 photographs
and the synthetic demo. Each image was reduced proportionally to a width of 48
pixels using OpenCV's area resampling. DP, greedy and graph cut used Sobel
energy and removed 4 columns followed by 3 rows.

For each method and image, the evaluator ran an untimed pass to record the
images and seam paths, followed by 3 timed passes. The CSV contains 54
timed records. Each photograph in the table links to its saved comparison panel.

## 5-photo results

Times are the median complete-resize time in milliseconds. Energy is the sum
of the 7 removed seams on their respective changing energy maps.

| Photo | Input | Target | DP (ms) | Greedy (ms) | Graph cut (ms) | DP energy | Greedy energy | Graph-cut energy |
|---|---|---|---:|---:|---:|---:|---:|---:|
| [Landmark](graphcut_results/eval/01_landmark/comparison.png) | 48 × 88 | 44 × 85 | 0.896 | 0.360 | 316.738 | 48,762 | 66,132 | 48,800 |
| [Landscape](graphcut_results/eval/02_landscape/comparison.png) | 48 × 38 | 44 × 35 | 0.301 | 0.099 | 30.496 | 40,976 | 50,214 | 41,824 |
| [Portrait](graphcut_results/eval/03_person/comparison.png) | 48 × 72 | 44 × 69 | 0.264 | 0.395 | 177.324 | 51,820 | 98,924 | 51,940 |
| [Texture](graphcut_results/eval/04_texture/comparison.png) | 48 × 32 | 44 × 29 | 0.134 | 0.114 | 40.009 | 79,098 | 111,658 | 82,640 |
| [Architecture](graphcut_results/eval/05_architecture/comparison.png) | 48 × 64 | 44 × 61 | 0.393 | 0.166 | 69.772 | 33,582 | 95,484 | 33,582 |

Greedy removed more energy than DP on all 5 photos. Graph cut matched all
7 DP paths on architecture and recorded a higher total cost on the other
4. The graph-cut implementation connects the final column directly to the
sink without pricing that column, so it can miss a cheaper boundary seam.
The initial CSV does not record the hardware or compiler used for this run.
Treat its times as measurements of that run rather than universal rankings.

The evaluator reports energy calculation, seam search, removal and total time.
The timed passes exclude image loading, preview preparation, overlay recording
and file output. Total time also includes session setup and validation. The
untimed pass records original-coordinate overlays and paths. Red marks
vertical seams and blue marks horizontal seams. `different_paths_vs_dp`
counts steps whose path differs from DP in that step's current image.

## Run the comparison

Build the project using the [project instructions](../README.txt), then run:

```powershell
$photos = @(Get-ChildItem data -Filter '*.jpg' | Sort-Object Name | ForEach-Object { $_.FullName })
.\build\native\Release\seam-evaluate.exe --max-width 48 --cols 4 --rows 3 --repeats 3 --out output/compare-48 @photos data/demo.png
```

The paths, dimensions, seam costs and pixel differences in the saved C++
results were reproduced with the current evaluator. Timing values will vary.

## Earlier comparisons

The [width-48](width-48/results.csv) and [width-80](width-80/results.csv)
records used C++ DP and greedy with a Python Dinic graph-cut implementation.
They are historical measurements from the earlier pipeline. Their graph-cut
timings and paths describe that implementation and include transfer through
the Python bridge. The corresponding panels are in those folders.

The separate [DP profile](dp-profile.csv) records 5 repetitions for the
rolling and full DP buffers and the Sobel worker configurations. The reported
DP bytes count the cost and parent buffers, rather than total process memory.

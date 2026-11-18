# C++ seam-carving comparison

The [algorithm comparison](algorithm-comparison.csv) covers 5 photographs
and the [synthetic demo](comparisons/demo.png). OpenCV's area resampling
reduced each image proportionally to a width of 48 pixels. DP, greedy and
graph cut used Sobel energy and removed 4 columns followed by 3 rows.

For each method and image, the evaluator ran an untimed pass to record the
images and seam paths, followed by 3 timed passes. The CSV contains 54
timed records. The [comparison panels](comparisons/) show the input, its Sobel
energy map, and each method's resized image and seam overlay.

## 5-photo results

Times are the median complete-resize time in milliseconds. Energy is the sum
of the 7 removed seams on their respective changing energy maps.

| Photo | Input | Target | DP (ms) | Greedy (ms) | Graph cut (ms) | DP energy | Greedy energy | Graph-cut energy |
|---|---|---|---:|---:|---:|---:|---:|---:|
| [Landmark](comparisons/01_landmark.png) | 48 × 88 | 44 × 85 | 0.896 | 0.360 | 316.738 | 48,762 | 66,132 | 48,800 |
| [Landscape](comparisons/02_landscape.png) | 48 × 38 | 44 × 35 | 0.301 | 0.099 | 30.496 | 40,976 | 50,214 | 41,824 |
| [Portrait](comparisons/03_person.png) | 48 × 72 | 44 × 69 | 0.264 | 0.395 | 177.324 | 51,820 | 98,924 | 51,940 |
| [Texture](comparisons/04_texture.png) | 48 × 32 | 44 × 29 | 0.134 | 0.114 | 40.009 | 79,098 | 111,658 | 82,640 |
| [Architecture](comparisons/05_architecture.png) | 48 × 64 | 44 × 61 | 0.393 | 0.166 | 69.772 | 33,582 | 95,484 | 33,582 |

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

The [DP memory and Sobel threading measurements](dp-memory-and-sobel-threading.csv)
give the mean time over 5 runs for each DP storage mode and Sobel worker
count. The byte counts cover the DP cost and parent buffers.

## Run the comparison

Build the project using the [project instructions](../README.txt), then run:

```powershell
$photos = @(Get-ChildItem data -Filter '*.jpg' | Sort-Object Name | ForEach-Object { $_.FullName })
.\build\native\Release\seam-evaluate.exe --max-width 48 --cols 4 --rows 3 --repeats 3 --out output/compare-48 @photos data/demo.png
```

The command writes `output/compare-48/results.csv` and a folder per input
with its individual PNGs and comparison panel. The current evaluator reproduced the saved dimensions,
seam costs, path comparisons and pixel differences. Timing values will vary.

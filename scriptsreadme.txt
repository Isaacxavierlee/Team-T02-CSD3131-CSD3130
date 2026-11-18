TEAM PRESENTATION SCRIPT - CURRENT REPOSITORY
=============================================

Use this script with the current team repository at commit 7555a4a or a later
version your team has reviewed. The project uses C++ and OpenCV for the main
app, DP and greedy, and a Python graph-cut solver for the three-method
evaluator. There are six named speakers below. Each has less than two minutes.
The brief allows twelve minutes for the whole team. Slides are optional.

BEFORE RECORDING

1. Get the Windows artifact from a successful GitHub Actions run as described
   in README.txt, extract it and open PowerShell in the extracted folder.
2. Prepare the architecture example by running:

   .\seam-carve.exe data/05_architecture.jpg --interactive --max-width 80 --width 72 --height 101

   The window may finish automatically. Press R to restart, then Space to pause.
   Press N to step. Red is a vertical seam; blue is a horizontal seam.
3. Have these repository files open: cpp/image.cpp, cpp/dp.cpp, cpp/greedy.cpp,
   cpp/resize.cpp, cpp/python_baseline.cpp and
   seam_carving/algorithms/graphcut.py. Also open
   evaluation/width-80/05_architecture.png and evaluation/README.md. Use the
   saved comparison for all three methods. The Windows GUI offers DP and greedy
   variants. It does not provide graph cut in the algorithm control.
4. Open the team's successful CI run and practice before recording. If the
   code or results change, update the facts and numbers below before recording.

ISAAC - IMAGE PROCESSING AND LIVE DEMONSTRATION (ABOUT 1 MINUTE)

Show the running OpenCV window. Move the width slider. Press R to restart,
Space to pause, then N twice to show a seam and its removal. Point to the
original overlay on the left and current image on the right.

"Our project resizes an image by removing connected seams. Each seam contains
one pixel per row or column, and neighbouring positions differ by at most one.
The window lets us choose the target size, algorithm and energy method. Red
marks vertical seams and blue marks horizontal ones. Here I pause, show a seam,
and advance once more to remove it. Changing a slider starts from the source
image again.

For our comparison we use Sobel energy. The C++ code sums the absolute
horizontal and vertical responses across the three colour channels. Strong
edges usually have higher cost, so an algorithm can avoid them where possible."

DALTON - DYNAMIC PROGRAMMING (ABOUT 1 MINUTE)

Show cpp/dp.cpp, especially the predecessor comparison, parent offsets and
backtracking in solve, followed by the two-row allocation in find_dp.

"Dynamic programming asks for the cheapest path to each pixel. We add its
energy to the smallest cost among the valid neighbours in the previous step.
At the last step we select the lowest-cost endpoint and trace the recorded
parents back to the beginning. The same code works with either orientation
through EnergyView.

For H by W pixels the search takes O(HW) time. We keep two rows of double
costs and one signed-byte parent offset per pixel, so the path can still be
reconstructed. Our small-map tests compare the result with exhaustive search."

RUI - GREEDY (ABOUT 45 SECONDS)

Show cpp/greedy.cpp and the architecture panel at
evaluation/width-80/05_architecture.png.

"Greedy starts at the cheapest pixel along the first edge, then takes the
cheapest reachable neighbour at each step. It cannot reconsider an earlier
choice. In the recorded architecture example, greedy cuts across portions of
the window grid, while DP's horizontal seams mostly run through the sky.

At the 80 by 107 starting size, shrinking to 72 by 101, the recorded DP energy
is 86,526 and greedy's is 321,096, about 3.71 times as much. Energy measures
our chosen local image cost, so we also compare the output images visually."

DEXTER - GRAPH CUT (ABOUT 1 MINUTE)

Show seam_carving/algorithms/graphcut.py, then the evaluator bridge in
cpp/python_baseline.cpp. Point to the graph-cut column in the saved comparison.

"The graph-cut comparison represents seam selection as a minimum source-to-
sink cut. Forward edge capacities carry pixel energies, while backward and
diagonal constraints make the cut correspond to a connected seam. The dummy
boundary lets the solution select the last column. A max-flow calculation gives
the minimum cut, and the reachable source side identifies the seam.

In this checkout, the evaluator calls the Python Dinic implementation through
the C++ bridge. The graph-cut result is not a choice in the native GUI. In the
recorded comparison, DP and graph cut chose identical paths and produced
identical pixels across the ten photo-and-size cases. Equal-cost paths can
differ on other inputs."

FADZLY - RESIZING, RESULTS AND TESTS (ABOUT 1 MINUTE)

Show cpp/resize.cpp and evaluation/width-80/results.csv. Optionally show
the successful Actions run and its native comparison artifact.

"The resize session checks that each path is in bounds and connected before
removing it. We then calculate a fresh energy map because the pixel neighbours
have changed. The comparison uses the same prepared input, Sobel energy and
target for all three methods. It records each method's output, seam energy and
separate stage timings.

The CSV includes three timed repetitions at each size. On the architecture
example, the recorded medians are approximately 1.05 milliseconds for C++ DP,
0.76 for C++ greedy and 8,582.90 for Python graph cut. The graph-cut timing
includes crossing the language boundary, so this is a comparison of these
implementations rather than a pure comparison of languages or algorithms."

RANVITHA - LIMITATIONS, CI AND CONCLUSION (ABOUT 45 SECONDS)

Show evaluation/README.md, then a successful run under GitHub Actions. Point
out the Windows artifact and native test job if they are available.

"These results cover five photos at two preview widths and should not be
generalised to every image or resize ratio. Graph cut and DP optimise the same
per-seam cost, but the best individual seams do not guarantee the best final
image after many removals. Our automated workflow builds and tests the project
and uploads a Windows package and examples. For this dataset, DP was a useful
balance of seam quality and running time."

FINISHING THE VIDEO

Upload the recording, check that the intended viewer can open it and place the
link in the final report. Every speaker should review the shared code and
report before the closed-book team quiz. Do not claim a feature or test ran in
the video unless you actually demonstrate it.

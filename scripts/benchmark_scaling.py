"""Owner: Dalton / Fadzly.  Spec 5b: does measured running time match the theory?
Times ONE seam search for each algorithm on growing images and fits time ~ n^slope (n = H*W).
Expect slope ~1 for DP (Theta(HW)), ~0.5-1 for greedy (Theta(H+W) + python overhead), >1 for graph cut.
Run: python scripts/benchmark_scaling.py [image] ; writes output/scaling.csv (+ scaling.png if matplotlib exists)"""
import csv, os, sys, time
import numpy as np
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from seam_carving import compute_energy
from seam_carving.algorithms import ALGORITHMS
from seam_carving.io_utils import read_image, downscale_to_width

path = sys.argv[1] if len(sys.argv) > 1 else "data/demo.png"
img = read_image(path)
plan = {"dp": [30, 60, 120, 240], "dp_loop": [30, 60, 120], "dp_lowmem": [30, 60, 120, 240],
        "greedy": [30, 60, 120, 240], "graphcut": [15, 20, 30, 40], "graphcut_ek": [15, 20, 30, 40]}
rows = []
for algo, widths in plan.items():
    for w in widths:
        small = downscale_to_width(img, w)
        e = compute_energy(small)
        reps = 3 if algo.startswith("graphcut") else 10
        t0 = time.perf_counter()
        for _ in range(reps):
            ALGORITHMS[algo](e)
        dt = (time.perf_counter() - t0) / reps
        rows.append({"algo": algo, "H": e.shape[0], "W": e.shape[1], "pixels": e.size, "seconds": dt})
        print(rows[-1])
os.makedirs("output", exist_ok=True)
with open("output/scaling.csv", "w", newline="") as f:
    wr = csv.DictWriter(f, fieldnames=rows[0].keys()); wr.writeheader(); wr.writerows(rows)
for algo in plan:
    r = [x for x in rows if x["algo"] == algo]
    if len(r) >= 2:
        slope = np.polyfit(np.log([x["pixels"] for x in r]), np.log([x["seconds"] for x in r]), 1)[0]
        print(f"{algo:12s} time ~ pixels^{slope:.2f}")
try:
    import matplotlib; matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    for algo in plan:
        r = [x for x in rows if x["algo"] == algo]
        plt.loglog([x["pixels"] for x in r], [x["seconds"] for x in r], "o-", label=algo)
    plt.xlabel("pixels (H*W)"); plt.ylabel("seconds per seam"); plt.legend(); plt.grid(True, which="both", alpha=.3)
    plt.savefig("output/scaling.png", dpi=150); print("saved output/scaling.png")
except ImportError:
    print("matplotlib not installed - CSV only")

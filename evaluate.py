"""EVALUATION / COMPARISON  -  owner: Fadzly     (Spec 3 + 4 "compare seams, visual quality, running time"; Spec 5c)

Runs DP, greedy and graph cut on the SAME images and sizes and writes:
  results.csv                    time (total/seam/energy), seam_cost, mean_abs_diff_vs_dp (0 = same picture as DP)
  <img>_c<k>_r<k>_compare.png    results [original | dp | greedy | graphcut] (top row), seams drawn (bottom row)
Usage:  python evaluate.py data/*.png --cols 10 20 --rows 5 --max-width 80
mean_abs_diff_vs_dp is a NUMBER for visual quality that supports (not replaces) looking at the pictures.
Graph cut is slow: keep --max-width small (60-100). Put real photos (e.g. Broadway Tower) in data/.
"""
import argparse
import csv
import os
import numpy as np
from seam_carving import carve
from seam_carving.algorithms import ALGORITHMS, MAIN_ALGORITHMS
from seam_carving.io_utils import read_image, save_image, downscale_to_width
from seam_carving.visualize import draw_seams, side_by_side


def main():
    p = argparse.ArgumentParser()
    p.add_argument("images", nargs="+")
    p.add_argument("--cols", type=int, nargs="+", default=[20])
    p.add_argument("--rows", type=int, nargs="+", default=[0])
    p.add_argument("--algos", nargs="+", default=list(MAIN_ALGORITHMS), choices=list(ALGORITHMS))
    p.add_argument("--energy", default="sobel")
    p.add_argument("--max-width", type=int, default=100)
    p.add_argument("--out", default="output/eval")
    a = p.parse_args()

    os.makedirs(a.out, exist_ok=True)
    rows = []
    for path in a.images:
        img = downscale_to_width(read_image(path), a.max_width)
        h, w = img.shape[:2]
        name = os.path.splitext(os.path.basename(path))[0]
        for c in a.cols:
            for r in a.rows:
                results, overlays = {}, {}
                for algo in a.algos:
                    res, st = carve(img, w - c, h - r, algorithm=algo, energy_method=a.energy, record_seams=True)
                    results[algo], overlays[algo] = res, draw_seams(img, st["seams"])
                    ref = results.get("dp")
                    diff = ""
                    if ref is not None and ref.shape == res.shape:
                        diff = round(float(np.abs(ref.astype(int) - res.astype(int)).mean()), 3)
                    rows.append({"image": name, "size": f"{w}x{h}", "cols": c, "rows": r, "algo": algo,
                                 "total_s": round(st["total_time"], 4), "seam_s": round(st["seam_time"], 4),
                                 "energy_s": round(st["energy_time"], 4), "seam_cost": round(st["seam_cost"], 1),
                                 "mean_abs_diff_vs_dp": diff})
                    print(rows[-1])
                top = side_by_side([("original", img)] + [(k, v) for k, v in results.items()])
                bot = side_by_side([("seams", img)] + [(k, v) for k, v in overlays.items()])
                save_image(f"{a.out}/{name}_c{c}_r{r}_compare.png", np.vstack([top, bot]))
    with open(f"{a.out}/results.csv", "w", newline="") as f:
        wr = csv.DictWriter(f, fieldnames=rows[0].keys())
        wr.writeheader()
        wr.writerows(rows)
    print("wrote", f"{a.out}/results.csv")


if __name__ == "__main__":
    main()

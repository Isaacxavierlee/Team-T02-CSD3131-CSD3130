"""SEAM REMOVAL + RESIZE LOOP  -  owner: Fadzly (written by Isaac)   (Spec 1c, Spec 5; Rubric: Implementation)

SPEC 1c  "After identifying the minimum energy seam, remove it. Then recalculate the energy map, and
          repeat until the image is resized to the desired dimensions."
  carve() does exactly that for ANY algorithm in ALGORITHMS:
      repeat:  energy = compute_energy(image)  ->  seam = find_seam(energy)  ->  image = remove_seam(image)
  * remove_seam(): boolean mask drops one pixel per row and reshapes to (H, W-1).        O(H*W)
  * Horizontal seams (Spec 1 "both vertical and horizontal"): transpose, apply the vertical code, transpose back.
  * order="vertical_first" removes all column seams, then all row seams; order="alternate" interleaves them
    (a cheap approximation of the optimal ordering of Avidan & Shamir - compare them in the report).
  * Energy is recomputed from scratch after every seam (matches the spec; see 5d for the incremental idea).
  * record_seams / on_step: every seam is also reported in ORIGINAL image coordinates (we carry a
    coordinate map through the same removals) -> used for the seam overlays and bonus 7a step-by-step view.
  * stats: energy_time, seam_time, total_time, seam_cost  -> the numbers Fadzly tabulates (Spec 3/4 comparisons).

COMPLEXITY for k seams: k * (energy O(HW) + seam-finder + removal O(HW)). With DP: O(k*H*W).
Only shrinking is implemented (the assignment is about compression). Enlarging = seam insertion (extra).
"""
import time
import numpy as np
from .energy import compute_energy
from .algorithms import ALGORITHMS


def remove_seam(arr: np.ndarray, seam: np.ndarray) -> np.ndarray:
    """Remove one pixel per row (vertical seam). Works for (H,W) or (H,W,C) arrays."""
    h, w = arr.shape[:2]
    mask = np.ones((h, w), dtype=bool)
    mask[np.arange(h), seam] = False
    return arr[mask].reshape((h, w - 1) + arr.shape[2:])


def _swap(arr):
    return np.swapaxes(arr, 0, 1)


def _plan(nv, nh, order):
    if order == "vertical_first":
        return ["vertical"] * nv + ["horizontal"] * nh
    if order == "alternate":
        ops, v, h = [], nv, nh
        while v or h:
            if v:
                ops.append("vertical"); v -= 1
            if h:
                ops.append("horizontal"); h -= 1
        return ops
    raise ValueError("order must be 'vertical_first' or 'alternate'")


def carve(img, target_w=None, target_h=None, algorithm="dp", energy_method="sobel",
          record_seams=False, on_step=None, order="vertical_first"):
    """Shrink img to (target_h, target_w). Returns (result, stats).

    stats keys: n_vertical, n_horizontal, energy_time, seam_time, total_time, seam_cost,
                seams = [(orientation, (N,2) array of original (row, col))] if record_seams.
    on_step(step, orientation, current_img, seam_in_original_coords) is called after each removal.
    """
    if algorithm not in ALGORITHMS:
        raise ValueError(f"Unknown algorithm '{algorithm}'. Options: {list(ALGORITHMS)}")
    find_seam = ALGORITHMS[algorithm]

    h0, w0 = img.shape[:2]
    target_w = w0 if target_w is None else target_w
    target_h = h0 if target_h is None else target_h
    if not (1 <= target_w <= w0 and 1 <= target_h <= h0):
        raise ValueError(f"Target {target_w}x{target_h} must be between 1x1 and {w0}x{h0} (shrink only).")

    stats = {"n_vertical": w0 - target_w, "n_horizontal": h0 - target_h,
             "energy_time": 0.0, "seam_time": 0.0, "seam_cost": 0.0, "seams": []}
    t_start = time.perf_counter()

    cur = img.copy()
    coords = np.stack(np.indices((h0, w0)), axis=-1) if (record_seams or on_step) else None

    for step, orientation in enumerate(_plan(w0 - target_w, h0 - target_h, order), start=1):
        vertical = orientation == "vertical"
        work = cur if vertical else _swap(cur)
        wcoords = None if coords is None else (coords if vertical else _swap(coords))

        t0 = time.perf_counter()
        energy = compute_energy(work, energy_method)
        t1 = time.perf_counter()
        seam = find_seam(energy)
        t2 = time.perf_counter()
        stats["energy_time"] += t1 - t0
        stats["seam_time"] += t2 - t1
        stats["seam_cost"] += float(energy[np.arange(len(seam)), seam].sum())

        seam_orig = None
        if wcoords is not None:
            seam_orig = wcoords[np.arange(len(seam)), seam]
            if record_seams:
                stats["seams"].append((orientation, seam_orig))
            wcoords = remove_seam(wcoords, seam)
            coords = wcoords if vertical else _swap(wcoords)

        work = remove_seam(work, seam)
        cur = np.ascontiguousarray(work if vertical else _swap(work))
        if on_step:
            on_step(step, orientation, cur, seam_orig)

    stats["total_time"] = time.perf_counter() - t_start
    return cur, stats

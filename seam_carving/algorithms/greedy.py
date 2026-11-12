"""GREEDY SEAM  -  owner: Rui                (Spec 3, Spec 5c; Rubric: Design, Analysis)

SPEC 3  "select the seam by making a local, row-by-row decision, choosing the minimum-energy pixel at
         each step, without considering the entire image."
  find_seam(): start at the cheapest pixel of row 0; in every next row move to the cheapest of the three
  pixels directly below (down-left / down / down-right). One pass, no table.

HOW IT DIFFERS FROM DP (Spec 5c): DP keeps the best cost of EVERY partial seam and decides at the end
  (global optimum). Greedy commits immediately and can never undo a choice, so a cheap pixel now may
  lead into a wall of expensive pixels (see scripts/greedy_failure_case.py: greedy cost 150 vs DP cost 4).
  Greedy is NOT optimal, but it is still a valid connected seam.

COMPLEXITY: time Theta(H + W) (scan row 0, then 3 look-ups per row); space O(1) extra besides the seam.
  -> much faster than DP's Theta(H*W), usually worse seams -> a speed/quality trade-off to measure
  ("Does the greedy algorithm give comparable visual quality?"; Spec 3).

find_seam_lookahead(k): extension - choose the next pixel by the best cost over the next k rows (k=0 is the
  plain greedy). Shows that more look-ahead moves quality towards DP at higher cost (~3^k per row).
IMPROVE / REPORT: quantify with evaluate.py (seam_cost, image difference) over several images.
"""
import numpy as np


def find_seam(energy: np.ndarray) -> np.ndarray:
    h, w = energy.shape
    seam = np.empty(h, dtype=np.int64)
    seam[0] = int(np.argmin(energy[0]))
    for i in range(1, h):
        j = seam[i - 1]
        lo, hi = max(0, j - 1), min(w, j + 2)
        seam[i] = lo + int(np.argmin(energy[i, lo:hi]))
    return seam


def _best_cost(energy, i, j, k):
    """Cheapest cost of a path that starts at (i, j) and continues for up to k more rows."""
    c = float(energy[i, j])
    if k == 0 or i == energy.shape[0] - 1:
        return c
    w = energy.shape[1]
    return c + min(_best_cost(energy, i + 1, jj, k - 1) for jj in range(max(0, j - 1), min(w, j + 2)))


def find_seam_lookahead(energy: np.ndarray, k: int = 3) -> np.ndarray:
    h, w = energy.shape
    seam = np.empty(h, dtype=np.int64)
    seam[0] = min(range(w), key=lambda j: _best_cost(energy, 0, j, k))
    for i in range(1, h):
        j = seam[i - 1]
        cands = range(max(0, j - 1), min(w, j + 2))
        seam[i] = min(cands, key=lambda jj: _best_cost(energy, i, jj, k))
    return seam

"""DYNAMIC PROGRAMMING SEAM  -  owner: Dalton    (Spec 1b, Spec 2a(ii), Spec 5b/5d; Rubric: Design, Analysis)

SPEC 1b  "Apply dynamic programming to find the seam with the minimum cumulative energy."
  Subproblem (answers Spec 2a(ii) "how to relate to subproblems"):
      M[i, j] = minimum total energy of any seam that starts in row 0 and ENDS at pixel (i, j)
      M[0, j] = e[0, j]
      M[i, j] = e[i, j] + min(M[i-1, j-1], M[i-1, j], M[i-1, j+1])      (neighbours inside the image)
  Optimal substructure: a cheapest seam ending at (i, j) must reach (i-1, j') by a cheapest seam
  ending there (otherwise swap in the cheaper one) -> the recurrence is exact.
  Answer = min_j M[H-1, j]; backtrack the stored choices to recover the seam.

FUNCTIONS
  cumulative_energy / trace_seam   the two phases (forward fill, backtrack)
  find_seam                        vectorised per row (used by the program; fast)
  find_seam_loop                   plain double loop, the version to explain in the report / quiz
  find_seam_low_memory             keeps only ONE row of M (Spec 5d: "do we need the entire DP table?")
  count_seams                      counts seams to illustrate Spec 2a(i) (exponential growth)

COMPLEXITY (Spec 5b), per seam, image H x W:
  time  Theta(H*W)  (each cell looks at <= 3 cells)       space  Theta(H*W)  (M float64 + int8 pointers)
  low-memory variant: values need only O(W); backpointers still need H*W int8 (1 byte/pixel instead of 8+)
  Greedy is Theta(H+W), graph cut is far larger -> see graphcut.py.
SPEC 5d answer: the table of VALUES need not be kept (only the previous row), but the BACKPOINTERS must
  be (or recompute them while backtracking). Threads: row i depends on row i-1, so the DP itself is serial;
  only the energy step parallelises (energy.sobel_energy_dc). Measure with scripts/experiment_threads.py.

IMPROVE: tie-break rule effects; early-exit variants; forward energy DP (changes the cost terms).
"""
import numpy as np


def cumulative_energy(energy: np.ndarray):
    """Forward phase. Returns (M, back) where back[i, j] in {-1, 0, +1} = column offset of the
    chosen predecessor in row i-1 (int8 to save memory)."""
    h, w = energy.shape
    M = energy.astype(np.float64).copy()
    back = np.zeros((h, w), dtype=np.int8)
    cols = np.arange(w)
    for i in range(1, h):
        prev = M[i - 1]
        cand = np.full((3, w), np.inf)
        cand[0, 1:] = prev[:-1]          # from upper-left
        cand[1] = prev                   # from above
        cand[2, :-1] = prev[1:]          # from upper-right
        choice = cand.argmin(axis=0)     # ties -> left, then up, then right
        M[i] += cand[choice, cols]
        back[i] = choice.astype(np.int8) - 1
    return M, back


def trace_seam(M: np.ndarray, back: np.ndarray) -> np.ndarray:
    """Backtrack phase: start at the cheapest cell of the last row and follow the pointers up."""
    h = M.shape[0]
    seam = np.empty(h, dtype=np.int64)
    seam[-1] = int(np.argmin(M[-1]))
    for i in range(h - 1, 0, -1):
        seam[i - 1] = seam[i] + back[i, seam[i]]
    return seam


def find_seam(energy: np.ndarray) -> np.ndarray:
    M, back = cumulative_energy(energy)
    return trace_seam(M, back)


def find_seam_loop(energy: np.ndarray) -> np.ndarray:
    """Textbook version with explicit loops (identical result, slower in Python)."""
    h, w = energy.shape
    M = [[0.0] * w for _ in range(h)]
    back = [[0] * w for _ in range(h)]
    M[0] = [float(x) for x in energy[0]]
    for i in range(1, h):
        for j in range(w):
            best, off = float("inf"), 0
            for d in (-1, 0, 1):
                jj = j + d
                if 0 <= jj < w and M[i - 1][jj] < best:
                    best, off = M[i - 1][jj], d
            M[i][j] = float(energy[i, j]) + best
            back[i][j] = off
    j = min(range(w), key=lambda c: M[h - 1][c])
    seam = [0] * h
    seam[h - 1] = j
    for i in range(h - 1, 0, -1):
        seam[i - 1] = seam[i] + back[i][seam[i]]
    return np.array(seam, dtype=np.int64)


def find_seam_low_memory(energy: np.ndarray) -> np.ndarray:
    """Same answer, but only the previous row of M is alive (Spec 5d)."""
    h, w = energy.shape
    prev = energy[0].astype(np.float64)
    back = np.zeros((h, w), dtype=np.int8)
    cols = np.arange(w)
    for i in range(1, h):
        cand = np.full((3, w), np.inf)
        cand[0, 1:] = prev[:-1]
        cand[1] = prev
        cand[2, :-1] = prev[1:]
        choice = cand.argmin(axis=0)
        prev = energy[i] + cand[choice, cols]
        back[i] = choice.astype(np.int8) - 1
    seam = np.empty(h, dtype=np.int64)
    seam[-1] = int(np.argmin(prev))
    for i in range(h - 1, 0, -1):
        seam[i - 1] = seam[i] + back[i, seam[i]]
    return seam


def count_seams(m: int, n: int) -> int:
    """Number of valid vertical seams in an m x n image (for Spec 2a(i)). For n = 2 it is 2**m."""
    c = [1] * n
    for _ in range(m - 1):
        c = [c[j] + (c[j - 1] if j > 0 else 0) + (c[j + 1] if j < n - 1 else 0) for j in range(n)]
    return sum(c)

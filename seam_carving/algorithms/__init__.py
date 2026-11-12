"""SEAM-FINDING ALGORITHMS - the contract every algorithm follows.

    find_seam(energy: float array (H, W)) -> int array (H,)
  seam[i] = column of the pixel removed in row i;  0 <= seam[i] < W  and  |seam[i] - seam[i-1]| <= 1.
Vertical seams only: carver.py transposes for horizontal seams (Spec 1 / Spec 4).

MAIN_ALGORITHMS are the three required by the spec (DP, greedy, graph cut) and are what evaluate.py
compares by default. ALGORITHMS also contains variants used for the analysis (Spec 5b-5d).
"""
from . import dp, greedy, graphcut

MAIN_ALGORITHMS = {
    "dp": dp.find_seam,                       # Spec 1
    "greedy": greedy.find_seam,               # Spec 3
    "graphcut": graphcut.find_seam,           # Spec 4 (Dinic max-flow)
}

ALGORITHMS = {
    **MAIN_ALGORITHMS,
    "dp_loop": dp.find_seam_loop,
    "dp_lowmem": dp.find_seam_low_memory,
    "greedy_look3": lambda e: greedy.find_seam_lookahead(e, 3),
    "graphcut_ek": graphcut.find_seam_edmonds_karp,
    "graphcut_nx": graphcut.find_seam_networkx,
}

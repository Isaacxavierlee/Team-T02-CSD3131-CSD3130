"""ENERGY MAPS  -  owner: Isaac            (Spec 1a, Spec 2b, Spec 5d; Rubric: Design, Implementation)

SPEC 1a  "Calculate the Energy Map ... based on the gradient ... or by using filters like Sobel."
  -> sobel_energy(): |Gx| + |Gy| with the two 3x3 Sobel kernels, summed over the colour channels.
     Written with numpy shifted slices (== a 3x3 convolution), so it is O(H*W) and we do not use
     any content-aware resizing function (forbidden by the spec).
  -> gradient_energy(): simpler finite differences (a baseline to compare against Sobel).
  -> forward_energy(): |I[i,j+1] - I[i,j-1]|, the cost formula quoted in Spec 4 (what the image would
     "tear" if pixel j were removed). Works with every algorithm because it is still a per-pixel map.

SPEC 2b  "Discuss whether Divide and Conquer could improve the efficiency of this [convolution]."
  -> sobel_energy_dc(): the experiment. Divide the image into row strips (with a 1-row halo so border
     pixels see their true neighbours), conquer = run Sobel on each strip (optionally in threads),
     combine = concatenate. Output is identical to sobel_energy (tested). Total work is still
     Theta(H*W): D&C gives NO asymptotic gain, only a possible parallel speed-up. Measure it with
     scripts/experiment_threads.py and say so in the report.

COMPLEXITY: every function here is Theta(H*W) time, Theta(H*W) extra space.
IMPROVE (ideas):  incremental update (after removing a seam only pixels within 1 column of it change),
  cv2.Sobel for speed comparison, true forward energy (Rubinstein et al. 2008) inside the DP.
"""
from concurrent.futures import ThreadPoolExecutor
import numpy as np


def _as3d(img):
    f = img.astype(np.float64)
    return f[:, :, None] if f.ndim == 2 else f


def sobel_energy(img: np.ndarray) -> np.ndarray:
    """|Gx|+|Gy| (3x3 Sobel), summed over channels. Edge pixels use replicated borders."""
    p = np.pad(_as3d(img), ((1, 1), (1, 1), (0, 0)), mode="edge")
    tl, t, tr = p[:-2, :-2], p[:-2, 1:-1], p[:-2, 2:]
    l, r = p[1:-1, :-2], p[1:-1, 2:]
    bl, b, br = p[2:, :-2], p[2:, 1:-1], p[2:, 2:]
    gx = (tr + 2 * r + br) - (tl + 2 * l + bl)
    gy = (bl + 2 * b + br) - (tl + 2 * t + tr)
    return (np.abs(gx) + np.abs(gy)).sum(axis=2)


def gradient_energy(img: np.ndarray) -> np.ndarray:
    """|dI/dx| + |dI/dy| with central differences (np.gradient)."""
    gy, gx = np.gradient(_as3d(img), axis=(0, 1))
    return (np.abs(gx) + np.abs(gy)).sum(axis=2)


def forward_energy(img: np.ndarray) -> np.ndarray:
    """|I[i,j+1] - I[i,j-1]| (Spec 4): the new horizontal edge created if pixel j is removed."""
    p = np.pad(_as3d(img), ((0, 0), (1, 1), (0, 0)), mode="edge")
    return np.abs(p[:, 2:] - p[:, :-2]).sum(axis=2)


def sobel_energy_dc(img: np.ndarray, parts: int = 4, threads: int = 1) -> np.ndarray:
    """Divide & conquer Sobel (Spec 2b). Same result as sobel_energy; see module docstring."""
    h = img.shape[0]
    parts = max(1, min(parts, h))
    bounds = np.linspace(0, h, parts + 1).astype(int)

    def conquer(k):
        a, b = bounds[k], bounds[k + 1]
        lo, hi = max(0, a - 1), min(h, b + 1)            # strip + 1-row halo
        e = sobel_energy(img[lo:hi])
        return e[a - lo: a - lo + (b - a)]               # drop halo rows

    if threads > 1:
        with ThreadPoolExecutor(threads) as ex:
            pieces = list(ex.map(conquer, range(parts)))
    else:
        pieces = [conquer(k) for k in range(parts)]
    return np.concatenate(pieces, axis=0)


ENERGY_FUNCS = {
    "sobel": sobel_energy,
    "gradient": gradient_energy,
    "forward": forward_energy,
    "sobel_dc": lambda img: sobel_energy_dc(img, parts=4, threads=4),
}


def compute_energy(img: np.ndarray, method: str = "sobel") -> np.ndarray:
    try:
        return ENERGY_FUNCS[method](img)
    except KeyError:
        raise ValueError(f"Unknown energy method '{method}'. Options: {list(ENERGY_FUNCS)}")


def energy_to_image(energy: np.ndarray) -> np.ndarray:
    """Normalise an energy map to uint8 greyscale (for the report figures)."""
    e = energy - energy.min()
    if e.max() > 0:
        e = e / e.max()
    return (e * 255).astype(np.uint8)

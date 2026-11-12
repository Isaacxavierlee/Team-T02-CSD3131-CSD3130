"""Owner: Dalton.  Spec 2b (divide & conquer for convolution), Spec 5d (multithreading, DP table memory).
Run: python scripts/experiment_threads.py
Prints (1) Sobel energy: single pass vs divide-and-conquer strips with 1/2/4/8 threads,
       (2) peak memory of the full-table DP vs the low-memory DP.
Report the numbers HONESTLY, even if threads do not help (small images: thread overhead dominates)."""
import os, sys, time, tracemalloc
import numpy as np
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from seam_carving.energy import sobel_energy, sobel_energy_dc
from seam_carving.algorithms import dp

rng = np.random.default_rng(0)
img = rng.integers(0, 255, (2000, 3000, 3)).astype(np.uint8)


def timeit(f, reps=3):
    t = time.perf_counter()
    for _ in range(reps):
        out = f()
    return (time.perf_counter() - t) / reps, out


t0, ref = timeit(lambda: sobel_energy(img))
print(f"single pass sobel          : {t0:.3f}s")
for th in (1, 2, 4, 8):
    t, out = timeit(lambda: sobel_energy_dc(img, parts=max(th, 2), threads=th))
    print(f"D&C strips, threads={th}      : {t:.3f}s  speed-up x{t0 / t:.2f}  identical={np.array_equal(out, ref)}")

e = sobel_energy(img[:1000, :1000])
for name, f in (("dp full table", dp.find_seam), ("dp low memory", dp.find_seam_low_memory)):
    tracemalloc.start(); f(e); _, peak = tracemalloc.get_traced_memory(); tracemalloc.stop()
    print(f"{name:14s} peak memory: {peak / 1e6:.1f} MB")

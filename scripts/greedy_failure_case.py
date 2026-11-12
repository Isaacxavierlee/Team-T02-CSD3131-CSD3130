"""Owner: Rui.  Spec 3/5c: a concrete case where GREEDY is much worse than DP.
Run: python scripts/greedy_failure_case.py   -> prints costs and writes output/greedy_failure.png"""
import os, sys
import cv2
import numpy as np
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from seam_carving.algorithms import dp, greedy

E = np.array([[0, 9, 1, 9, 9],
              [50, 50, 1, 50, 50],
              [50, 50, 1, 50, 50],
              [50, 50, 1, 50, 50]], dtype=float)   # a cheap column hidden behind a '0' decoy in row 0


def cost(s):
    return E[np.arange(len(s)), s].sum()


g, d = greedy.find_seam(E), dp.find_seam(E)
print("greedy seam", g, "cost", cost(g))
print("dp     seam", d, "cost", cost(d))
cell = 60
img = cv2.applyColorMap((255 - np.kron(np.clip(E * 5, 0, 255), np.ones((cell, cell)))).astype(np.uint8), cv2.COLORMAP_BONE)
for seam, col in ((g, (0, 0, 255)), (d, (0, 160, 0))):
    pts = [(int(j * cell + cell / 2), int(i * cell + cell / 2)) for i, j in enumerate(seam)]
    cv2.polylines(img, [np.array(pts)], False, col, 4)
os.makedirs("output", exist_ok=True)
cv2.imwrite("output/greedy_failure.png", img)
print("saved output/greedy_failure.png (red = greedy, green = DP)")

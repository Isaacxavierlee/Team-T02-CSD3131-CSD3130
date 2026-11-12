"""Generate a synthetic test image (sky, ground, a 'tower') at data/demo.png."""
import os
import numpy as np
import cv2

h, w = 160, 240
img = np.zeros((h, w, 3), np.uint8)
img[:100] = (200, 120, 40)
img[100:] = (50, 140, 60)
cv2.rectangle(img, (150, 20), (180, 110), (120, 150, 170), -1)
cv2.rectangle(img, (160, 40), (170, 55), (30, 30, 30), -1)
cv2.circle(img, (40, 30), 12, (240, 240, 250), -1)
rng = np.random.default_rng(0)
img = np.clip(img.astype(int) + rng.integers(-6, 6, img.shape), 0, 255).astype(np.uint8)
os.makedirs("data", exist_ok=True)
cv2.imwrite("data/demo.png", img)

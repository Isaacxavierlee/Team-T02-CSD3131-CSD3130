"""IMAGE INPUT / OUTPUT  -  owner: Isaac      (Spec 1 "reading/writing images" via OpenCV, which the spec allows)

read_image / save_image: images are numpy arrays (H, W, 3) uint8 in OpenCV's BGR order. Everything else in
the project only sees numpy arrays, so any teammate can test with synthetic arrays (no files needed).
downscale_to_width: ordinary resize used ONLY to make test images small enough for the slow graph cut.
It is never part of the seam-carving result (using library resizing for the result would be 'cheating').
"""
import os
import cv2
import numpy as np


def read_image(path: str) -> np.ndarray:
    img = cv2.imread(path, cv2.IMREAD_COLOR)
    if img is None:
        raise FileNotFoundError(f"Could not read image: {path}")
    return img


def save_image(path: str, img: np.ndarray) -> None:
    folder = os.path.dirname(path)
    if folder:
        os.makedirs(folder, exist_ok=True)
    if not cv2.imwrite(path, np.clip(img, 0, 255).astype(np.uint8)):
        raise IOError(f"Could not write image: {path}")


def downscale_to_width(img: np.ndarray, max_width: int) -> np.ndarray:
    h, w = img.shape[:2]
    if w <= max_width:
        return img
    return cv2.resize(img, (max_width, max(1, round(h * max_width / w))), interpolation=cv2.INTER_AREA)

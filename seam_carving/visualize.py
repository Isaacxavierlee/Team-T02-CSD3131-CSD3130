"""VISUALISATION  -  owner: Isaac / Fadzly      (Bonus 7a step-by-step visualisation with ALL seams; report figures)

draw_seams          paint removed seams on the original (vertical = red, horizontal = blue)
progress_frames     frame k = original with the first k seams drawn -> shows the ORDER seams are chosen
save_frames/video   write frames as PNGs / an mp4 (CLI: --frames-dir, --video)
side_by_side        labelled comparison strip (original | DP | greedy | graph cut) for the report (Spec 3/4)
"""
import os
import cv2
import numpy as np


def draw_seams(img, seams, colors=None):
    out = img.copy()
    colors = colors or {"vertical": (0, 0, 255), "horizontal": (255, 0, 0)}
    for orientation, pts in seams:
        out[pts[:, 0], pts[:, 1]] = colors[orientation]
    return out


def progress_frames(img, seams, every=1):
    for k in range(every, len(seams) + 1, every):
        yield draw_seams(img, seams[:k])


def save_frames(img, seams, folder, every=1):
    os.makedirs(folder, exist_ok=True)
    n = 0
    for k, frame in enumerate(progress_frames(img, seams, every), start=1):
        cv2.imwrite(os.path.join(folder, f"seam_{k * every:04d}.png"), frame)
        n += 1
    return n


def save_video(img, seams, path, fps=15, every=1):
    h, w = img.shape[:2]
    os.makedirs(os.path.dirname(path) or ".", exist_ok=True)
    vw = cv2.VideoWriter(path, cv2.VideoWriter_fourcc(*"mp4v"), fps, (w, h))
    if not vw.isOpened():
        raise IOError("Could not open video writer (mp4v codec missing?) - use --frames-dir instead")
    for frame in progress_frames(img, seams, every):
        vw.write(frame)
    vw.release()


def pad_to(img, h, w, value=128):
    out = np.full((h, w) + img.shape[2:], value, dtype=img.dtype)
    out[: img.shape[0], : img.shape[1]] = img
    return out


def side_by_side(items, gap=6):
    """items = [(label, image), ...] -> one image, all panels padded to the same size, labels on top."""
    h = max(im.shape[0] for _, im in items)
    w = max(im.shape[1] for _, im in items)
    panels = []
    for label, im in items:
        p = np.full((h + 22, w, 3), 255, np.uint8)
        p[22:, :] = pad_to(im, h, w)
        cv2.putText(p, label, (4, 15), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 0, 0), 1, cv2.LINE_AA)
        panels.append(p)
        panels.append(np.full((h + 22, gap, 3), 255, np.uint8))
    return np.hstack(panels[:-1])

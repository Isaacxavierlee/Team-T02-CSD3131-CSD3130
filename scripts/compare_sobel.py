import sys, time
import numpy as np, cv2
sys.path.insert(0, ".")
from seam_carving.energy import sobel_energy
from seam_carving.io_utils import read_image

img = read_image(sys.argv[1] if len(sys.argv) > 1 else "data/01_landmark.jpg")

def cv_sobel(im):
    f = im.astype(np.float64)
    e = 0
    for c in range(f.shape[2]):
        gx = cv2.Sobel(f[:, :, c], cv2.CV_64F, 1, 0, ksize=3)
        gy = cv2.Sobel(f[:, :, c], cv2.CV_64F, 0, 1, ksize=3)
        e = e + np.abs(gx) + np.abs(gy)
    return e

def timeit(f, n=20):
    t = time.perf_counter()
    for _ in range(n):
        out = f(img)
    return (time.perf_counter() - t) / n, out

t1, a = timeit(sobel_energy)
t2, b = timeit(cv_sobel)
print(f"ours {t1*1000:.1f} ms | cv2.Sobel {t2*1000:.1f} ms")
print("interior identical:", np.allclose(a[1:-1, 1:-1], b[1:-1, 1:-1]))
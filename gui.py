"""DESKTOP APP (tkinter)  -  owner: Isaac     (Bonus 7b: user interactively edits image size; Spec 8c: demo the app)
Pick an image, algorithm, energy and target size with sliders; see the original with all removed seams next to
the result, the running time and the seam cost. Work runs in a background thread so the window never freezes.
Launch with run.bat / run.sh, or: python gui.py
"""
import base64
import queue
import threading
import tkinter as tk
from tkinter import filedialog, messagebox, ttk

import cv2

from seam_carving import ALGORITHMS, carve
from seam_carving.energy import ENERGY_FUNCS
from seam_carving.io_utils import downscale_to_width, read_image, save_image
from seam_carving.visualize import draw_seams

PREVIEW_W, PREVIEW_H = 460, 340


def to_photo(img):
    """BGR numpy image -> tk.PhotoImage, scaled down to fit the preview (display only)."""
    h, w = img.shape[:2]
    s = min(PREVIEW_W / w, PREVIEW_H / h, 1.0)
    if s < 1.0:
        img = cv2.resize(img, (max(1, int(w * s)), max(1, int(h * s))), interpolation=cv2.INTER_AREA)
    ok, buf = cv2.imencode(".png", img)
    return tk.PhotoImage(data=base64.b64encode(buf.tobytes()))


class App:
    def __init__(self, root):
        self.root = root
        root.title("Seam Carving")
        self.orig = self.img = self.result = None
        self.q = queue.Queue()

        ctl = ttk.Frame(root, padding=10)
        ctl.grid(row=0, column=0, sticky="ns")
        ttk.Button(ctl, text="Open image...", command=self.open_image).pack(fill="x")
        self.path_lbl = ttk.Label(ctl, text="No image loaded", wraplength=200)
        self.path_lbl.pack(anchor="w", pady=(2, 10))

        ttk.Label(ctl, text="Algorithm").pack(anchor="w")
        self.algo = tk.StringVar(value="dp")
        ttk.Combobox(ctl, textvariable=self.algo, values=list(ALGORITHMS), state="readonly").pack(fill="x")
        ttk.Label(ctl, text="Energy").pack(anchor="w", pady=(8, 0))
        self.energy = tk.StringVar(value="sobel")
        ttk.Combobox(ctl, textvariable=self.energy, values=list(ENERGY_FUNCS), state="readonly").pack(fill="x")

        ttk.Label(ctl, text="Max input width (0 = off; use ~80 for graphcut)").pack(anchor="w", pady=(8, 0))
        self.maxw = tk.StringVar(value="0")
        e = ttk.Entry(ctl, textvariable=self.maxw)
        e.pack(fill="x")
        e.bind("<Return>", lambda _e: self.prepare())
        e.bind("<FocusOut>", lambda _e: self.prepare())

        self.w_lbl = ttk.Label(ctl, text="Target width")
        self.w_lbl.pack(anchor="w", pady=(10, 0))
        self.w_var = tk.IntVar(value=1)
        self.w_scale = tk.Scale(ctl, orient="horizontal", variable=self.w_var, from_=1, to=1,
                                command=lambda v: self.update_labels())
        self.w_scale.pack(fill="x")
        self.h_lbl = ttk.Label(ctl, text="Target height")
        self.h_lbl.pack(anchor="w")
        self.h_var = tk.IntVar(value=1)
        self.h_scale = tk.Scale(ctl, orient="horizontal", variable=self.h_var, from_=1, to=1,
                                command=lambda v: self.update_labels())
        self.h_scale.pack(fill="x")

        self.run_btn = ttk.Button(ctl, text="Run seam carving", command=self.run)
        self.run_btn.pack(fill="x", pady=(10, 2))
        self.save_btn = ttk.Button(ctl, text="Save result...", command=self.save, state="disabled")
        self.save_btn.pack(fill="x")
        self.status = ttk.Label(ctl, text="Open an image to start.", wraplength=200, foreground="#444")
        self.status.pack(anchor="w", pady=10)

        view = ttk.Frame(root, padding=10)
        view.grid(row=0, column=1)
        ttk.Label(view, text="Original + removed seams (red/blue)").grid(row=0, column=0)
        ttk.Label(view, text="Result").grid(row=0, column=1)
        self.left = ttk.Label(view)
        self.left.grid(row=1, column=0, padx=5)
        self.right = ttk.Label(view)
        self.right.grid(row=1, column=1, padx=5)
        self.poll()

    # ---- image handling
    def open_image(self):
        path = filedialog.askopenfilename(filetypes=[("Images", "*.png *.jpg *.jpeg *.bmp *.webp")])
        if not path:
            return
        try:
            self.orig = read_image(path)
        except Exception as ex:
            messagebox.showerror("Error", str(ex))
            return
        self.path_lbl.config(text=path.split("/")[-1].split("\\")[-1])
        self.prepare()

    def prepare(self):
        if self.orig is None:
            return
        try:
            mw = int(self.maxw.get() or 0)
        except ValueError:
            mw = 0
        self.img = downscale_to_width(self.orig, mw) if mw > 0 else self.orig
        h, w = self.img.shape[:2]
        self.w_scale.config(to=w)
        self.h_scale.config(to=h)
        self.w_var.set(w)
        self.h_var.set(h)
        self.photo_l = to_photo(self.img)
        self.left.config(image=self.photo_l)
        self.right.config(image="")
        self.update_labels()
        self.status.config(text=f"Loaded {w}x{h}")

    def update_labels(self):
        if self.img is None:
            return
        h, w = self.img.shape[:2]
        self.w_lbl.config(text=f"Target width: {self.w_var.get()} (remove {w - self.w_var.get()})")
        self.h_lbl.config(text=f"Target height: {self.h_var.get()} (remove {h - self.h_var.get()})")

    # ---- run in a background thread so the window stays responsive
    def run(self):
        if self.img is None:
            messagebox.showinfo("Seam carving", "Open an image first.")
            return
        algo, energy = self.algo.get(), self.energy.get()
        tw, th = self.w_var.get(), self.h_var.get()
        h, w = self.img.shape[:2]
        if algo == "graphcut" and (w > 120 or h > 120) and not messagebox.askyesno(
                "Graph cut is slow", "This image is large; graph cut could take a very long time.\nContinue?"):
            return
        self.run_btn.config(state="disabled")
        self.status.config(text="Working...")
        img = self.img

        def work():
            try:
                res, st = carve(img, tw, th, algorithm=algo, energy_method=energy, record_seams=True)
                self.q.put(("ok", res, draw_seams(img, st["seams"]), st))
            except Exception as ex:
                self.q.put(("err", str(ex)))

        threading.Thread(target=work, daemon=True).start()

    def poll(self):
        try:
            msg = self.q.get_nowait()
        except queue.Empty:
            msg = None
        if msg:
            self.run_btn.config(state="normal")
            if msg[0] == "ok":
                _, self.result, seams_img, st = msg
                self.photo_l = to_photo(seams_img)
                self.photo_r = to_photo(self.result)
                self.left.config(image=self.photo_l)
                self.right.config(image=self.photo_r)
                self.save_btn.config(state="normal")
                self.status.config(text=f"Done in {st['total_time']:.2f}s "
                                        f"(seams {st['seam_time']:.2f}s). Seam cost {st['seam_cost']:.0f}")
            else:
                self.status.config(text="Error: " + msg[1])
        self.root.after(100, self.poll)

    def save(self):
        path = filedialog.asksaveasfilename(defaultextension=".png", filetypes=[("PNG", "*.png")])
        if path:
            save_image(path, self.result)


if __name__ == "__main__":
    root = tk.Tk()
    App(root)
    root.mainloop()

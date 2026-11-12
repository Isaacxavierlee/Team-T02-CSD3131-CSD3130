"""COMMAND-LINE ENTRY POINT  -  owner: Isaac     (Spec 1 + 8c "demonstrate your app")
Examples:
  python main.py data/demo.png --algo dp --remove-cols 40 --remove-rows 20
  python main.py data/demo.png --algo graphcut --max-width 60 --remove-cols 8 --remove-rows 4
  python main.py data/demo.png --algo dp --remove-cols 30 --frames-dir output/frames --video output/seams.mp4
Writes <out>_<algo>.png (result), <out>_<algo>_seams.png (all seams on the original), <out>_energy.png.
"""
import argparse
from seam_carving import carve, compute_energy, ALGORITHMS
from seam_carving.energy import ENERGY_FUNCS, energy_to_image
from seam_carving.io_utils import read_image, save_image, downscale_to_width
from seam_carving.visualize import draw_seams, save_frames, save_video


def main():
    p = argparse.ArgumentParser(description="Seam carving (shrink)")
    p.add_argument("image")
    p.add_argument("--algo", choices=list(ALGORITHMS), default="dp")
    p.add_argument("--energy", choices=list(ENERGY_FUNCS), default="sobel")
    p.add_argument("--order", choices=["vertical_first", "alternate"], default="vertical_first")
    p.add_argument("--width", type=int, help="target width")
    p.add_argument("--height", type=int, help="target height")
    p.add_argument("--remove-cols", type=int, default=0, help="number of vertical seams to remove")
    p.add_argument("--remove-rows", type=int, default=0, help="number of horizontal seams to remove")
    p.add_argument("--max-width", type=int, help="downscale input first (needed for graph cut)")
    p.add_argument("--out", default="output/result", help="output prefix (no extension)")
    p.add_argument("--frames-dir", help="save step-by-step seam frames here (bonus 7a)")
    p.add_argument("--video", help="save step-by-step seam video (mp4) here (bonus 7a)")
    a = p.parse_args()

    img = read_image(a.image)
    if a.max_width:
        img = downscale_to_width(img, a.max_width)
    h, w = img.shape[:2]
    tw = a.width if a.width else w - a.remove_cols
    th = a.height if a.height else h - a.remove_rows

    result, st = carve(img, tw, th, algorithm=a.algo, energy_method=a.energy,
                       record_seams=True, order=a.order)
    save_image(f"{a.out}_{a.algo}.png", result)
    save_image(f"{a.out}_{a.algo}_seams.png", draw_seams(img, st["seams"]))
    save_image(f"{a.out}_energy.png", energy_to_image(compute_energy(img, a.energy)))
    if a.frames_dir:
        print("frames saved:", save_frames(img, st["seams"], a.frames_dir))
    if a.video:
        save_video(img, st["seams"], a.video)
    print(f"{a.algo}: {w}x{h} -> {tw}x{th} | total {st['total_time']:.2f}s "
          f"(energy {st['energy_time']:.2f}s, seam {st['seam_time']:.2f}s) | seam cost {st['seam_cost']:.0f}")


if __name__ == "__main__":
    main()

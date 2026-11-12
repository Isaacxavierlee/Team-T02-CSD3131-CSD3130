# Seam Carving - Assignment 2 (complete pipeline)

**Quick start (Windows):** double-click `run.bat`  (Mac/Linux: `./run.sh`). It updates the code, sets up Python libraries and opens the app.

Command line:
```bash
pip install -r requirements.txt
python scripts/make_demo_image.py
python main.py data/demo.png --algo dp --remove-cols 40 --remove-rows 20          # algos: dp, greedy, graphcut (+ variants)
python main.py data/demo.png --algo graphcut --max-width 60 --remove-cols 8      # graph cut is slow: small images
python main.py data/demo.png --algo dp --remove-cols 30 --frames-dir output/frames --video output/seams.mp4   # bonus 7a
python evaluate.py data/*.png --cols 10 20 --rows 5 --max-width 80               # comparison tables + images
python scripts/benchmark_scaling.py                                              # time vs size (Spec 5b)
python scripts/experiment_threads.py                                             # D&C / threads / memory (Spec 2b, 5d)
python scripts/greedy_failure_case.py                                            # greedy vs DP counter-example
pytest -q
```

## Start here
* `docs/BABYSIT_ISAAC.md` - day-by-day plan (team lead)
* `docs/TEAM_GUIDE.md` - what each person owns and how to improve it
* `docs/SPEC_TRACEABILITY.md` - every assignment requirement -> file -> proof
* `docs/THEORY_ANSWERS.md`, `docs/REPORT_TEMPLATE.md`, `docs/PRESENTATION_PLAN.md`, `docs/AI_DECLARATION_TEMPLATE.md`, `docs/QUIZ_PREP.md`

## Files
| File | Owner | Spec |
|---|---|---|
| `seam_carving/io_utils.py`, `energy.py`, `visualize.py`, `main.py`, `gui.py`, `run.bat` | Isaac | 1a, 2b, 7a, 7b |
| `seam_carving/algorithms/dp.py` | Dalton | 1b, 2a, 5b, 5d |
| `seam_carving/algorithms/greedy.py` | Rui | 3 |
| `seam_carving/algorithms/graphcut.py` | Dexter | 4 |
| `seam_carving/carver.py`, `evaluate.py`, `tests/` | Fadzly | 1c, 3, 4 comparison |
| `docs/` | Ranvitha | 2, 5, 8, 9 |

#!/usr/bin/env bash
# Mac / Linux launcher. First time: chmod +x run.sh
cd "$(dirname "$0")" || exit 1
if command -v git >/dev/null && [ -d .git ]; then git pull --ff-only || echo "Could not update - continuing."; fi
command -v python3 >/dev/null || { echo "Install Python 3 first (https://www.python.org)"; exit 1; }
[ -d .venv ] || python3 -m venv .venv || exit 1
if ! cmp -s requirements.txt .venv/req.stamp; then
  .venv/bin/python -m pip install -q -r requirements.txt && cp requirements.txt .venv/req.stamp || exit 1
fi
[ -f data/demo.png ] || .venv/bin/python scripts/make_demo_image.py
if [ $# -eq 0 ]; then .venv/bin/python gui.py; else .venv/bin/python main.py "$@"; fi

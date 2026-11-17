#!/usr/bin/env bash
set -eu
cd "$(dirname "$0")"

cmake -S . -B build/native -DCMAKE_BUILD_TYPE=Release
cmake --build build/native --config Release --target seam-carve

app=build/native/seam-carve
if [[ -x build/native/Release/seam-carve ]]; then
    app=build/native/Release/seam-carve
fi
exec "$app" "$@"

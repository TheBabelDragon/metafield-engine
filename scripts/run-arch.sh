#!/usr/bin/env bash
# Hands-free Arch Linux entry point for metafield-engine.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

if ! command -v cmake >/dev/null 2>&1 || { ! command -v g++ >/dev/null 2>&1 && ! command -v c++ >/dev/null 2>&1; }; then
  echo "missing cmake or C++ compiler"
  echo "install with: sudo pacman -S --needed base-devel cmake git"
  exit 1
fi

mkdir -p build
cd build
if [[ ! -f CMakeCache.txt ]]; then
  cmake ..
fi
cmake --build . --target hello_world hello_csi -j"$(nproc)"

echo
echo "=== hello_world ==="
./hello_world
echo
echo "=== hello_csi (8 seconds; Ctrl+C to stop early) ==="
echo "If /tmp/metafield/csi.jsonl exists it will be followed; otherwise synthetic."
echo
./hello_csi --seconds 8
echo
echo "Live follow (until Ctrl+C):"
echo "  $ROOT/build/hello_csi"

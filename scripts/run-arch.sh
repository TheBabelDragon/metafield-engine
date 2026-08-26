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
cmake --build . --target hello_world hello_csi hello_view -j"$(nproc)"

echo
echo "=== hello_world ==="
./hello_world
echo
echo "=== visual HUD ==="
echo "Opening http://127.0.0.1:8765"
echo "If /tmp/metafield/csi.jsonl exists it is followed; otherwise synthetic."
echo "Ctrl+C stops the view."
echo

PORT=8765
./hello_view --port "$PORT" &
VIEW_PID=$!
cleanup() { kill "$VIEW_PID" 2>/dev/null || true; }
trap cleanup EXIT INT TERM

sleep 0.4
if command -v xdg-open >/dev/null 2>&1; then
  xdg-open "http://127.0.0.1:${PORT}" >/dev/null 2>&1 || true
elif command -v firefox >/dev/null 2>&1; then
  firefox "http://127.0.0.1:${PORT}" >/dev/null 2>&1 || true
else
  echo "Open this in a browser: http://127.0.0.1:${PORT}"
fi

wait "$VIEW_PID"

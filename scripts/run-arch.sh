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
# Always reconfigure so new targets (hello_view) appear in stale build trees.
cmake ..
cmake --build . --target hello_world hello_csi hello_view scarcity_clock_test -j"$(nproc)"

echo
echo "=== hello_world ==="
./hello_world
echo
echo "=== scarcity clock ==="
./scarcity_clock_test
echo
echo "=== visual HUD ==="
PORT=8765
URL="http://127.0.0.1:${PORT}"
echo
echo "  HUD  ${URL}"
echo
echo "If /tmp/metafield/csi.jsonl exists it is followed; otherwise synthetic."
echo "Ctrl+C stops the view."
echo

./hello_view --port "$PORT" &
VIEW_PID=$!
cleanup() { kill "$VIEW_PID" 2>/dev/null || true; }
trap cleanup EXIT INT TERM

sleep 0.4
if command -v xdg-open >/dev/null 2>&1; then
  xdg-open "$URL" >/dev/null 2>&1 || true
elif command -v firefox >/dev/null 2>&1; then
  firefox "$URL" >/dev/null 2>&1 || true
fi

wait "$VIEW_PID"

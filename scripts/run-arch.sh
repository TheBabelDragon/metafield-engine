#!/usr/bin/env bash
# Hands-free Arch Linux entry point for metafield-engine.
# Hardware required: ESP32 CSI node writing /tmp/metafield/csi.jsonl.
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
cmake ..
cmake --build . --target hello_world hello_csi hello_view scarcity_clock_test world_kernel_test -j"$(nproc)"

echo
echo "=== hello_world ==="
./hello_world
echo
echo "=== world kernel ==="
./world_kernel_test
echo
echo "=== scarcity clock ==="
./scarcity_clock_test
echo
echo "=== visual HUD (live ESP32 only) ==="
PORT=8765
URL="http://127.0.0.1:${PORT}"
echo
echo "  HUD  ${URL}"
echo
echo "synthetic_cyd is gone. Need a physical node:"
echo "  1. Flash wifi-sensing-system ESP32 or CYD"
echo "  2. python3 ../scripts/csi-bridge.py     # UDP 4210 -> /tmp/metafield/csi.jsonl"
echo "Optical body: flash optical-body-s3 (real ADS1115, no fake ADC)."
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

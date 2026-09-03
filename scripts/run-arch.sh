#!/usr/bin/env bash
# Build + listen for a real CYD/ESP32 + HUD.
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"
JSONL="${METAFIELD_CSI_JSONL:-/tmp/metafield/csi.jsonl}"
PORT="${HUD_PORT:-8765}"
CSI_PORT="${CSI_PORT:-4210}"
if ! command -v cmake >/dev/null 2>&1 || { ! command -v g++ >/dev/null 2>&1 && ! command -v c++ >/dev/null 2>&1; }; then
  echo "missing cmake or C++ compiler"
  echo "install with: sudo pacman -S --needed base-devel cmake git python"
  exit 1
fi
mkdir -p "$(dirname "$JSONL")" build
touch "$JSONL"
cmake -S . -B build
cmake --build build --target hello_world hello_view world_kernel_test -j"$(nproc)"
echo
./build/hello_world
echo
./build/world_kernel_test
echo
BRIDGE_PID=""; VIEW_PID=""
cleanup() { [[ -n "$VIEW_PID" ]] && kill "$VIEW_PID" 2>/dev/null || true; [[ -n "$BRIDGE_PID" ]] && kill "$BRIDGE_PID" 2>/dev/null || true; }
trap cleanup EXIT INT TERM
SERIAL_ARGS=()
if [[ -n "${METAFIELD_CSI_SERIAL:-}" ]]; then SERIAL_ARGS+=(--serial "$METAFIELD_CSI_SERIAL"); fi
python3 "$ROOT/scripts/csi-bridge.py" --port "$CSI_PORT" --out "$JSONL" "${SERIAL_ARGS[@]+"${SERIAL_ARGS[@]}"}" &
BRIDGE_PID=$!
sleep 0.3
if ! kill -0 "$BRIDGE_PID" 2>/dev/null; then
  echo "csi-bridge died — UDP $CSI_PORT is probably taken."
  echo "If dashboard.py --csi is running, it must write $JSONL (updated dashboard does)."
fi
echo
echo "================================================"
echo " LIVE CYD + HUD"
echo "  HUD     http://127.0.0.1:${PORT}"
echo "  jsonl   ${JSONL}"
echo "  UDP     0.0.0.0:${CSI_PORT}"
echo "  host    announces on UDP ${CSI_PORT%*}4211"
echo "  serial  ${METAFIELD_CSI_SERIAL:-none}"
echo "================================================"
echo
"$ROOT/build/hello_view" --file "$JSONL" --port "$PORT" &
VIEW_PID=$!
sleep 0.4
if command -v xdg-open >/dev/null 2>&1; then xdg-open "http://127.0.0.1:${PORT}" >/dev/null 2>&1 || true; fi
wait "$VIEW_PID"

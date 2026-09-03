#!/usr/bin/env bash
# Build + listen for live CYD CSI and C3 swarm nodes + HUD.
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
echo "serial ports:"
ls -l /dev/ttyACM* /dev/ttyUSB* 2>/dev/null || echo "  none — plug a C3 USB for C3 bodies"
echo
BRIDGE_PID=""; C3_PID=""; VIEW_PID=""
cleanup() {
  [[ -n "$VIEW_PID" ]] && kill "$VIEW_PID" 2>/dev/null || true
  [[ -n "$BRIDGE_PID" ]] && kill "$BRIDGE_PID" 2>/dev/null || true
  [[ -n "$C3_PID" ]] && kill "$C3_PID" 2>/dev/null || true
}
trap cleanup EXIT INT TERM
SERIAL_ARGS=()
if [[ -n "${METAFIELD_CSI_SERIAL:-}" ]]; then SERIAL_ARGS+=(--serial "$METAFIELD_CSI_SERIAL"); fi
python3 "$ROOT/scripts/csi-bridge.py" --port "$CSI_PORT" --out "$JSONL" "${SERIAL_ARGS[@]+"${SERIAL_ARGS[@]}"}" &
BRIDGE_PID=$!
python3 "$ROOT/scripts/c3-bridge.py" &
C3_PID=$!
sleep 0.3
echo
echo "================================================"
echo " LIVE CYD + C3 SWARM + HUD"
echo "  HUD     http://127.0.0.1:${PORT}"
echo "  jsonl   ${JSONL}"
echo "  UDP     0.0.0.0:${CSI_PORT}   (CYD CSI)"
echo "  serial  C3 /dev/ttyACM* /dev/ttyUSB*"
echo "================================================"
echo
"$ROOT/build/hello_view" --file "$JSONL" --port "$PORT" &
VIEW_PID=$!
sleep 0.4
if command -v xdg-open >/dev/null 2>&1; then xdg-open "http://127.0.0.1:${PORT}" >/dev/null 2>&1 || true; fi
wait "$VIEW_PID"

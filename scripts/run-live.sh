#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"
JSONL="${METAFIELD_CSI_JSONL:-/tmp/metafield/csi.jsonl}"
PORT="${HUD_PORT:-8765}"
CSI_PORT="${CSI_PORT:-4210}"
if ! command -v cmake >/dev/null || { ! command -v g++ >/dev/null && ! command -v c++ >/dev/null; }; then
  echo "install: sudo pacman -S --needed base-devel cmake git python"
  exit 1
fi
mkdir -p "$(dirname "$JSONL")" build
touch "$JSONL"
cmake -S . -B build
cmake --build build --target hello_view -j"$(nproc)"
BRIDGE_PID=""; VIEW_PID=""
cleanup() { [[ -n "$VIEW_PID" ]] && kill "$VIEW_PID" 2>/dev/null || true; [[ -n "$BRIDGE_PID" ]] && kill "$BRIDGE_PID" 2>/dev/null || true; }
trap cleanup EXIT INT TERM
python3 "$ROOT/scripts/csi-bridge.py" --port "$CSI_PORT" --out "$JSONL" &
BRIDGE_PID=$!
echo
echo "================================================"
echo " LIVE CSI + HUD"
echo "  HUD     http://127.0.0.1:${PORT}"
echo "  jsonl   ${JSONL}"
echo "  UDP     0.0.0.0:${CSI_PORT}"
echo "  keys    arrows / WASD after clicking the canvas"
echo "  pass    badge LIVE and packets climb"
echo "================================================"
echo
"$ROOT/build/hello_view" --live --file "$JSONL" --port "$PORT" &
VIEW_PID=$!
sleep 0.5
if command -v xdg-open >/dev/null; then xdg-open "http://127.0.0.1:${PORT}" >/dev/null 2>&1 || true; fi
wait "$VIEW_PID"

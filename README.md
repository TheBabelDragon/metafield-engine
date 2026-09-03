# MetaField Engine

Physical-field substrate first. Simulation is one consumer.

**Kernel:** World admits Observations. Events in, ticks out, history is replayable. A cell is an address. See `docs/FOUNDATION.md`.

**Provenance:** physical / derived / synthetic. Unlabeled is synthetic. Synthetic is rejected unless `METAFIELD_ALLOW_SYNTHETIC=1`.

**Clock:** `btc_height` is the coarse epoch. Cumulative work is scarcity weight. Bitcoin is one `ExternalClockAnchor`.

## Arch Linux

```bash
cd metafield-engine
git pull
bash scripts/run-arch.sh
```

The process prints `http://127.0.0.1:8765`. That HUD is a frontend. House/tree/player are interpretations, not kernel types.

`jsonl [missing]` means no physical CSI yet. The HUD may still draw a labeled synthetic model. That model is not World-admitted field state.

Optional Bitcoin tip:

```bash
export METAFIELD_BTC_HEIGHT=900001
```

LIVE (optional other terminal):

```bash
mkdir -p /tmp/metafield
python -m observer.metafield_bridge --udp --out /tmp/metafield/csi.jsonl
```

Kernel check:

```bash
cmake -S . -B build && cmake --build build --target world_kernel_test && ./build/world_kernel_test
```

## What v0 includes

- Quantity + Provenance + Observation
- World kernel: Event → Tick → History → hash/replay
- Grid discretization (VoxelField) as one storage strategy
- CSI ingest tagged physical vs synthetic
- HUD frontend

Not yet: adaptive meshes, hardware topology graphs, Vulkan, Aurora.

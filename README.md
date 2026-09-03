# MetaField Engine

Cross-domain universe engine. World state first. Existing optical / echo / CSI / Aurora work plugs in later.

**Clock:** `btc_height` is the coarse epoch. Cumulative work is scarcity weight. Wall time is observed_at only. Unanchored Worlds are valid. See `docs/SCARCITY_CLOCK.md`.

## Arch Linux

```bash
cd metafield-engine
git pull
bash scripts/run-arch.sh
```

Stale build folder (the old `No rule to make hello_view` error):

```bash
cd metafield-engine/build
cmake ..
cmake --build . --target hello_view
./hello_view
```

The process prints:

```
http://127.0.0.1:8765
```

That is HTTP on localhost. Open it. Ctrl+C stops.

`jsonl [missing]` is normal until throne-room writes `/tmp/metafield/csi.jsonl`. The World still runs on a synthetic field.

Optional Bitcoin tip (still not confirmed):

```bash
export METAFIELD_BTC_HEIGHT=900001
```

or drop `/tmp/metafield/btc_clock.json`.

LIVE (optional other terminal):

```bash
mkdir -p /tmp/metafield
python -m observer.metafield_bridge --udp --out /tmp/metafield/csi.jsonl
```

## What v0 includes

- World + ECS + Time + Fields + scarcity clock
- First universe: house, tree, light, player, NPC
- CSI ingest (live JSONL or synthetic)
- 3D HUD of World-owned entities
- Player / NPC locomotion

Not yet: Vulkan renderer, optical-body / echo-grid adapters, Aurora fabric.

# MetaField Engine

**Unreal-style cross-domain universe engine.**

Canonical World state · ECS · Fields · CSI ingest · live visual HUD.

---

## Hands-free on Arch Linux

```bash
sudo pacman -S --needed base-devel cmake git
git clone https://github.com/TheBabelDragon/metafield-engine.git
cd metafield-engine
bash scripts/run-arch.sh
```

Already cloned:

```bash
cd metafield-engine
git pull
bash scripts/run-arch.sh
```

That builds the core, runs the smoke test, starts the HUD, and opens:

**http://127.0.0.1:8765**

No extra libraries. No UDP :4210 bind. Ctrl+C stops it.

### What you should see

- LIVE / SYN badge
- subcarrier bars
- RSSI / energy / spread meters
- field map with each CSI body
- body list + packet counts

If `/tmp/metafield/csi.jsonl` exists (throne-room / metafield_bridge), the HUD is live.
If not, it synthesizes an 8 Hz field so the picture still moves.

### Manual

```bash
mkdir -p build && cd build
cmake .. && cmake --build .
./hello_view
# then open http://127.0.0.1:8765
```

---

## Status

| Piece | State |
|-------|--------|
| World + ECS + Time | Working |
| CSI JSONL ingest | Working |
| Local visual HUD | Working |
| SDL/Vulkan renderer | Not started |
| Optical / Echo plugins | Placeholders |

See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md).

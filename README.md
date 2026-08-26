# MetaField Engine

**Unreal-style cross-domain universe engine.**

Canonical World state · ECS · multi-physics Fields · Hardware abstraction · Aurora Fabric · deterministic simulation.

Existing work (`optical-body-s3`, `echo-grid-ultrasonic-os`, Aurora, field-bus, CSI snake, …) becomes **plugins / adapters** around a common world model — not the other way around.

---

## Hands-free on Arch Linux

```bash
sudo pacman -S --needed base-devel cmake git
git clone https://github.com/TheBabelDragon/metafield-engine.git
cd metafield-engine
./scripts/run-arch.sh
```

That script installs nothing extra, builds the core, runs `hello_world`, then runs `hello_csi` for 8 seconds.

### CSI ingest

`hello_csi` is automatic:

| Situation | What happens |
|-----------|----------------|
| `/tmp/metafield/csi.jsonl` exists | Follow it live (throne-room / metafield_bridge output) |
| File missing | Synthetic 8 Hz CSI so the binary still runs |
| File appears later | Switches from synthetic → live |
| Throne Room already on :4210 | Engine does **not** bind the port |

Override path:

```bash
export METAFIELD_CSI_JSONL=/tmp/metafield/csi.jsonl
./build/hello_csi
```

Leave it running. The terminal is the first visual: RSSI / energy / spread bars + subcarrier sparkline.

---

## Manual build

```bash
mkdir -p build && cd build
cmake ..
cmake --build .
./hello_world
./hello_csi --seconds 8
```

---

## Repository Layout

```
metafield-engine/
├── engine/           # World, ECS, Fields, Time, ingest
├── plugins/csi/      # CSI JSONL adapter notes
├── examples/
│   ├── hello-world/
│   └── hello-csi/
├── scripts/run-arch.sh
└── docs/ARCHITECTURE.md
```

---

## Current Status

| Piece                    | State |
|--------------------------|--------|
| World + TimeState        | Working |
| ECS                      | Working |
| Analytic field stubs     | Working |
| CSI JSONL ingest         | Working (live file or synthetic) |
| Terminal field view      | Working |
| Renderer                 | Not started |
| Optical / Echo plugins   | Placeholders |

See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md).

---

*Part of the MetaField physical-field substrate work.*

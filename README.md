# MetaField Engine

**Unreal-style cross-domain universe engine.**

Canonical World state · ECS · multi-physics Fields · Hardware abstraction · Aurora Fabric · deterministic simulation.

Existing work (`optical-body-s3`, `echo-grid-ultrasonic-os`, Aurora, field-bus, …) becomes **plugins / adapters** around a common world model — not the other way around.

---

## Core Philosophy

1. **World State first.** Everything else (renderer, physics, hardware, network) is a consumer of a single deterministic representation of the universe.
2. **Fields are first-class.** An entity does not just have a transform; it can sample the continuous fields that surround it (optical, thermal, acoustic, EM, fluid, custom…).
3. **Plugins, not forks.** Physical bodies (optical, ultrasonic, Hall, ZVS…) and distributed compute (Aurora) attach via clean interfaces.
4. **Determinism.** Fixed timestep, stable IDs, versioned rules, hashable world state. Required for distributed simulation and replay.

---

## Repository Layout

```
metafield-engine/
├── engine/
│   ├── core/          # Math, types, logging, IDs
│   ├── ecs/           # Entity / Component / System
│   ├── world/         # World, TimeState, registries
│   ├── fields/        # Field interface + concrete fields
│   ├── physics/       # (future)
│   ├── renderer/      # (future — Vulkan/SDL/ImGui first)
│   ├── networking/    # (future)
│   ├── fabric/        # Aurora resource layer (future)
│   ├── scheduler/     # Work distribution (future)
│   ├── hardware/      # Device abstraction (future)
│   └── ai/            # (future)
├── editor/            # Viewport, Inspector, Hierarchy, Console
├── runtime/           # client / server / worker
├── plugins/
│   ├── optical-body/  # adapter → optical-body-s3
│   ├── echo-grid/     # adapter → echo-grid-ultrasonic-os
│   └── aurora/        # fabric integration
├── examples/
├── assets/
├── tests/
├── docs/
├── CMakeLists.txt
└── README.md
```

---

## Build & Run on Arch Linux

### 1. System packages

```bash
sudo pacman -S --needed base-devel cmake git
```

That is all. No extra libraries are required for the current core + hello-world.

### 2. Clone

```bash
git clone https://github.com/TheBabelDragon/metafield-engine.git
cd metafield-engine
```

### 3. Configure + build

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

Expected output ends with:

```
[100%] Built target hello_world
```

### 4. Run the smoke test

```bash
./hello_world
```

You should see something like:

```
Fields registered: 3
Living entities: 3
Optical intensity at player:  1
Thermal value at player:      24 °C
Acoustic intensity at player: 0.05
Simulation time: 0.0833333 s
Tick count:      5
Entities with Transform:
  Entity … @ (…)
  …

[OK] MetaField Engine core smoke test passed.
```

(Entity print order is currently unordered-map order — that is expected.)

### 5. Optional: run via CTest

```bash
ctest --output-on-failure
```

---

## Current Status

| Piece                    | State                          |
|--------------------------|--------------------------------|
| World + TimeState        | Working                        |
| ECS (EntityRegistry)     | Working (simple sparse pools)  |
| Field interface + stubs  | Working (analytic samples)     |
| hello-world example      | Builds & runs                  |
| Renderer                 | Not started                    |
| Hardware abstraction     | Not started                    |
| Plugins (optical/echo)   | Placeholders only              |
| Aurora Fabric            | Not started                    |

The architecture is now fixed so the next layers (renderer → hardware → plugins) can be added without rewriting the core.

See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for the full plan.

---

## Next milestones

1. Minimal renderer (SDL + Vulkan or OpenGL + ImGui) — spawn cube → move cube → render cube
2. HardwareDevice interface
3. `plugins/optical-body` adapter around the existing optical-body-s3 repo
4. One real ESP32 sensor feeding a FieldSample into the World

---

*Part of the MetaField physical-field substrate work.*

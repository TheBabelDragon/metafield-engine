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

## First Milestone (this commit)

- [x] `World` — single source of truth
- [x] `Entity` + sparse component storage (ECS foundation)
- [x] `Field` interface + sample API
- [x] `TimeState` (simulation / real / network / replay)
- [ ] Spawn cube → move cube → render cube
- [ ] 10k entities
- [ ] Sample a field and visualise it
- [ ] One real ESP32 sensor → world state

---

## Building

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

Requires C++20.

---

## Status

Architecture scaffold. No renderer, no hardware drivers, no networking yet.
The next concrete step is a minimal runtime that can spawn entities, tick the world, and sample fields.

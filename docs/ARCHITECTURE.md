# MetaField Engine — Architecture

## Goal

Turn the existing Babel / Aurora / MetaField work into an actual Unreal-style **cross-domain universe engine**.

This is a **new repository**. We do not mutate `optical-body-s3` into an engine.

## Absolute First Principle: World State

Everything depends on one canonical representation of the simulated universe:

```
World
{
    TimeState       time;
    EntityRegistry  entities;
    FieldRegistry   fields;
    // ResourceRegistry resources;   // later
};
```

Entities are thin. Data lives in components. Continuous phenomena live in Fields.

## ECS

```cpp
auto tree = world.spawn();
world.entities().add<Transform>(tree, {...});
world.entities().add<Renderable>(tree, {...});
world.entities().add<Thermal>(tree, {...});   // later
```

Systems operate on component views. New domains (optical, acoustic, thermal, AI, hardware) are added by adding components + systems — not by rewriting the engine.

## Fields (the MetaField difference)

```cpp
class Field {
    virtual FieldType type() const = 0;
    virtual void sample(const Vec3& position, FieldSample& out) const = 0;
};
```

An entity does not merely *have* a position. It can query its environment:

```cpp
world.sample(FieldType::Optical,  position);
world.sample(FieldType::Thermal,  position);
world.sample(FieldType::Acoustic, position);
```

Concrete fields (OpticalField, ThermalField, …) may be analytic, voxel grids, GPU, or **live hardware adapters**.

## Time

Multiple clocks exist because the engine must support:

- real-time interactive
- accelerated / slowed simulation
- network synchronisation
- physical sensor capture timestamps
- deterministic replay

## Future Layers (not in this commit)

| Layer | Role |
|-------|------|
| Renderer | Vulkan/SDL/ImGui first. Spawn cube → render cube. |
| Hardware | `HardwareDevice` interface → ESP32 / CAN / Serial / Optical body drivers |
| Optical-body plugin | Adapter around existing `optical-body-s3` |
| Echo-grid plugin | Adapter around `echo-grid-ultrasonic-os` |
| Aurora Fabric | Distributed resource layer (where is this? who has it? can I verify it?) |
| Scheduler | Decide which machine (local / worker / Aurora node) runs a WorkUnit |
| Network | Boring binary protocol first: WORLD_SNAPSHOT, ENTITY_UPDATE, WORK_REQUEST, … |
| Determinism | Fixed timestep, stable IDs, world hash (SHA-256 / BLAKE3) |
| Editor | Scene hierarchy + Inspector that shows field samples live |

## First Playable Universe (target)

100 m × 100 m world  
1 house · 1 tree · 1 light · 1 NPC  
1 simulated thermal field · 1 simulated optical field  
1 real ESP32 sensor  
1 Aurora node  
1 player

Player walks near tree → engine samples fields → ESP32 measurement enters world → Aurora records state → another node can reproduce it.

That is the proof of concept. Everything else is scaling the same abstractions.

## Plugin Boundary

```
MetaField Engine
    │
OpticalField  (interface)
    │
OpticalBodyAdapter  (plugin)
    │
optical-body-s3     (existing repo)
    │
ESP32 + BPW34 + laser
```

Same pattern for Echo Grid, Hall nodes, ZVS, WiFi CSI, etc.

## Current Status

This commit establishes only:

- `World`
- ECS (`EntityRegistry` + component pools)
- `Field` interface + three stub fields
- `TimeState`

No graphics. No hardware. No networking. The architecture is now fixed so those can be added without rewriting the core.

# MetaField implementation taxonomy

Simulation substrate first. Game engine second.
Minecraft-style voxels are one frontend on this graph.

```
META_FIELD
├── 00. KERNEL              [partial: clock lives in World::Time]
├── 01. SPATIAL SUBSTRATE   [now: CellCoord, ChunkCoord, VoxelField]
├── 02. CELL MODEL          [now: Channel + CellState]
├── 03. FIELD DYNAMICS      [now: DiffusionSystem]
├── 04. MATERIAL SYSTEM     planned
├── 05. ENERGY SYSTEM       planned
├── 06. INFORMATION FIELD   channel reserved
├── 07. PHYSICS             planned
├── 08. SIMULATION GRAPH    [now: FieldScheduler]
├── 09. ECS                 [v0 EntityRegistry]
├── 10. PROCEDURAL GEN      planned
├── 11. WORLD PERSISTENCE   planned (consume FieldDelta)
├── 12. OBSERVER MODEL      CSI / HUD is an observer
├── 13. RENDERING           HUD visualizes; voxel meshing later
├── 14. COMPUTE             planned
├── 15. AGENT SYSTEM        planned
├── 16. NETWORK FABRIC      Aurora later (consume FieldDelta)
├── 17. TOOLING             hello_field slice printer
└── 18. META-LAYER          planned (RuleSet as data)
```

## Fundamental abstraction

```
Field
 └── Chunk
      └── Cell
           ├── State
           ├── Properties
           └── Channels
```

Not Block.

## Causal spine

```
simulation
    ↓
FieldDelta { cell, channel, old, new }
    ↓
┌──────────────├──────────────├──────────────┐
│ renderer     │ persistence  │ networking   │
└──────────────┴──────────────┴──────────────┘
```

`FieldSystem::evaluate(FieldView, FieldDeltaList, dt)` never writes the field
directly. The scheduler applies the combined delta.

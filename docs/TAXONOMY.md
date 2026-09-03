# MetaField implementation taxonomy

Physical-field substrate first. Simulation is one consumer.
A voxel grid is one discretization of Field(domain, quantity, t).

```
META_FIELD
├── 00. KERNEL              World / Event / Tick / History / Clock / Provenance
├── 01. DOMAIN               coordinate systems (grid is one)
├── 02. QUANTITY             measurable scalars/vectors + units
├── 03. SAMPLE / OBSERVATION provenance required
├── 04. FIELD                quantity over a domain
├── 05. DISCRETIZATION       Chunk / Cell storage (optional)
├── 06. TRANSFORMATION       systems emit FieldDelta
├── 07. SIMULATION GRAPH     one consumer of field state
├── 08. ECS                  frontend participants, not competitors
├── 09. OBSERVER MODEL       CSI / HUD consume ticks
├── 10. HARDWARE TOPOLOGY    sensors define their own graph
├── 11. PERSISTENCE          consume history
├── 12. RENDERING            frontend
├── 13. NETWORK FABRIC       Aurora later
└── 14. META-LAYER           RuleSet as data
```

No material catalog. No block types.

## Causal spine

```
Observation / WorldEvent
        ↓
     systems
        ↓
    FieldDelta { address, quantity-channel, old, new }
        ↓
 renderer / persistence / network / actuators
```

# Foundation — physical-field substrate

Physical-field substrate first. Simulation is one consumer.

A cell is an address. It is not an object. Stone, dirt, water, wood, block, item
are interpretations. The kernel does not know them.

```
physical environment
        ↓
     sensors
        ↓
    Observation  (quantity, value, unit, uncertainty, provenance, when, optional where)
        ↓
      WORLD
        ↓
     Field state
        ├── inference (derived)
        ├── simulation (synthetic model)
        └── actuators / display / archive
```

## Source classes

- **physical** — instrument, sensor, external observation
- **derived** — computed from physical observations
- **synthetic** — explicitly simulated; rejected by World unless `METAFIELD_ALLOW_SYNTHETIC=1`

Unlabeled values are treated as synthetic.

## Kernel vocabulary

Domain · Coordinate · Quantity · Sample · Field · Observation · Event · Provenance · Time · History

Chunk / Cell / voxel / mesh / ECS / HUD are discretizations or frontends.

`Channel::Matter` stores mass density. It is not a material id.

## Every admitted value answers

WHAT? WHERE? WHEN? FROM WHAT? MEASURED OR DERIVED? BY WHOM? WITH WHAT UNCERTAINTY?

## Invariants

I. WorldState is the only authority.
II. Systems emit deltas. No hidden mutation.
III. Events, observations, deltas, ticks are history.
IV. Time is injectable. Bitcoin is one ExternalClockAnchor.
V. Same state + inputs + clock + rules → same hash.
VI. Synthetic cannot silently become measured.

See `engine/kernel/` and `tests/world_kernel.cpp`.

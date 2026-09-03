# MetaField Engine — Architecture

Physical-field substrate first. Simulation is one consumer.

## Layers

1. **Observation + Provenance** — what was measured, inferred, or modeled
2. **World kernel** — Event → Tick → History
3. **VoxelField** — one optional grid discretization (Field / Chunk / Cell / Channel)
4. **World ECS + HUD** — frontend sitting on the field

CSI and the HUD are observers. They do not own the universe.
House / tree / player in `hello_view` are interpretations, not substrate types.

See `docs/FOUNDATION.md` and `docs/TAXONOMY.md`.

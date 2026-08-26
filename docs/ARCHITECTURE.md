# MetaField Engine — Architecture

Simulation substrate first. Game engine second.

## Two layers

1. **VoxelField** — Field / Chunk / Cell / Channel. Rules write FieldDelta.
2. **World ECS** — entities (house, player, CSI body) that sit on the field.

CSI and the HUD are observers. They do not own the universe.

## v0.2 in this repo

- `engine/substrate/` FieldDelta spine + DiffusionSystem
- `examples/hello-field` heat slice
- World + ECS + CSI HUD (`hello_view`)

Does not bind UDP :4210.

See `docs/TAXONOMY.md` for the full 00–18 map.

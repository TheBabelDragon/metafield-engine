# MetaField Engine — Architecture

World is the single source of truth. Renderer, hardware, and Aurora consume it.

## v0 (this repo)

- World + ECS + Time + Fields
- Demo universe seeded in World: house, tree, light, player, NPC
- CSI ingest from `/tmp/metafield/csi.jsonl` or synthetic fallback
- Sensors become World entities
- Local HUD at http://127.0.0.1:8765 draws those entities

Does not bind UDP :4210.

## Not in v0

Vulkan/SDL native renderer · optical-body / echo-grid adapters · Aurora fabric · distributed scheduler.

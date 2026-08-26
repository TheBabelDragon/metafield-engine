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
};
```

Entities are thin. Data lives in components. Continuous phenomena live in Fields.

## Fields

An entity can query its environment:

```cpp
world.sample(FieldType::Optical,  position);
world.sample(FieldType::Thermal,  position);
world.sample(FieldType::Electromagnetic, position); // live CSI when ingested
```

## CSI ingest (implemented)

The engine consumes the existing snake path. It does not steal UDP :4210.

```
CYD / bridge → throne-room metafield_bridge → /tmp/metafield/csi.jsonl
                                                    ↓
                                              parse_csi_line
                                                    ↓
                                                 CsiField
                                                    ↓
                              world.sample(FieldType::Electromagnetic, pos)
```

Accepted line formats:

- raw `wifi_csi` `{node,rssi,type,csi[]}`
- MetaField FieldObservation `{body_id, body_type, field_regions, modality}`

If the JSONL is absent, `hello_csi` emits synthetic packets at 8 Hz so the binary is always runnable.

## Next

Renderer (SDL/ImGui) · HardwareDevice · optical-body / echo-grid adapters on the same Observation contract.

#pragma once

#include "engine/core/types.hpp"
#include "engine/ecs/registry.hpp"
#include "engine/world/time.hpp"
#include "engine/fields/field.hpp"

#include <string>

namespace mf {

// ---------------------------------------------------------------------------
// World — the single canonical representation of the simulated universe.
// Everything else (renderer, hardware, network, Aurora) reads/writes this.
// ---------------------------------------------------------------------------

class World {
public:
    World() = default;

    // --- Accessors ---------------------------------------------------------
    TimeState&       time()       { return time_; }
    const TimeState& time() const { return time_; }

    EntityRegistry&       entities()       { return entities_; }
    const EntityRegistry& entities() const { return entities_; }

    FieldRegistry&       fields()       { return fields_; }
    const FieldRegistry& fields() const { return fields_; }

    SimulationVersion&       version()       { return version_; }
    const SimulationVersion& version() const { return version_; }

    // --- High-level helpers ------------------------------------------------
    EntityID spawn() {
        return entities_.spawn();
    }

    void destroy(EntityID id) {
        entities_.destroy(id);
    }

    // Tick the simulation clock. Systems are called by the runtime later.
    void tick(double dt_real) {
        time_.advance(dt_real);
    }

    // Convenience field sampling
    FieldSample sample(FieldType type, const Vec3& position) const {
        return fields_.sample(type, position);
    }

    // Future: world hash for determinism / consensus
    // uint64_t hash() const;

private:
    TimeState         time_;
    EntityRegistry    entities_;
    FieldRegistry     fields_;
    SimulationVersion version_;
};

} // namespace mf

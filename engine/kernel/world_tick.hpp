#pragma once
#include "engine/kernel/world_clock.hpp"
#include "engine/kernel/world_event.hpp"
#include "engine/kernel/state_hash.hpp"
#include "engine/substrate/delta.hpp"
#include <cstdint>
#include <vector>
namespace mf {
using WorldId = std::uint64_t;
using TickSequence = std::uint64_t;
struct WorldTick {
    WorldId world = 1;
    TickSequence sequence = 0;
    double simulation_time = 0.0;
    float dt = 0.f;
    WorldClock clock;
    std::vector<WorldEvent> inputs;
    FieldDeltaList field_deltas;
    std::vector<EntityDelta> entity_deltas;
    StateHash state_hash = 0;
};
} // namespace mf

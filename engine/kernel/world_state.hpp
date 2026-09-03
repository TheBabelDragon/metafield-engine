#pragma once
#include "engine/kernel/state_hash.hpp"
#include "engine/kernel/world_tick.hpp"
#include "engine/substrate/field.hpp"
namespace mf {
struct WorldState {
    WorldId world = 1;
    WorldClock clock;
    FieldSnapshot substrate;
    std::uint64_t living_entities = 0;
    StateHash hash = 0;
    void rehash() { hash = hash_world_core(substrate, clock, living_entities); }
};
inline WorldState capture_world_state(WorldId world, const VoxelField& field, const WorldClock& clock, std::uint64_t living_entities) {
    WorldState s;
    s.world = world;
    s.clock = clock;
    s.substrate = capture(field);
    s.living_entities = living_entities;
    s.rehash();
    return s;
}
} // namespace mf

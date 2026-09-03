#pragma once
#include "engine/kernel/world_tick.hpp"
#include "engine/kernel/world_state.hpp"
#include "engine/substrate/field.hpp"
#include <vector>
namespace mf {
struct WorldHistory {
    WorldState genesis;
    std::vector<WorldTick> ticks;
    void append(WorldTick t) { ticks.push_back(std::move(t)); }
    std::size_t size() const { return ticks.size(); }
};
inline VoxelField replay_substrate(const WorldState& genesis, const WorldHistory& history) {
    VoxelField field;
    restore(field, genesis.substrate);
    for (const auto& t : history.ticks) field.apply(t.field_deltas);
    return field;
}
} // namespace mf

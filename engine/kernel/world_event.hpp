#pragma once
#include "engine/core/types.hpp"
#include "engine/substrate/coord.hpp"
#include "engine/substrate/channel.hpp"
#include <cstdint>
#include <string>
namespace mf {
enum class EventType : uint8_t { Impulse = 0, Observation, Command, Spawn, Destroy };
struct WorldEvent {
    std::uint64_t sequence = 0;
    EntityID source = INVALID_ENTITY;
    EventType type = EventType::Impulse;
    std::string name;
    CellCoord cell{};
    Channel channel = Channel::Energy;
    float value = 0.f;
};
enum class EntityOp : uint8_t { Spawn = 0, Destroy, Write };
struct EntityDelta {
    EntityOp op = EntityOp::Write;
    EntityID id = INVALID_ENTITY;
    std::string tag;
    float value = 0.f;
};
} // namespace mf

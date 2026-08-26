#pragma once

#include "engine/core/types.hpp"
#include <bitset>
#include <cstdint>

namespace mf {

// Maximum number of distinct component types the engine supports.
// Raise this only when necessary — it affects ComponentMask size.
constexpr size_t MAX_COMPONENTS = 64;

using ComponentMask = std::bitset<MAX_COMPONENTS>;

// ---------------------------------------------------------------------------
// Entity is intentionally thin. All data lives in component pools.
// ---------------------------------------------------------------------------

struct Entity {
    EntityID      id          = INVALID_ENTITY;
    ComponentMask components;
    bool          alive       = false;
};

} // namespace mf

#pragma once

#include <cstdint>

namespace mf {

// ---------------------------------------------------------------------------
// Fundamental IDs — stable across simulation, network, and replay
// ---------------------------------------------------------------------------

using EntityID    = uint64_t;
using ComponentID = uint32_t;
using FieldID     = uint32_t;
using AssetID     = uint64_t;
using WorkID      = uint64_t;

constexpr EntityID INVALID_ENTITY = 0;

// ---------------------------------------------------------------------------
// Math primitives (minimal — expand later or pull in glm)
// ---------------------------------------------------------------------------

struct Vec3 {
    float x = 0.f, y = 0.f, z = 0.f;

    constexpr Vec3() = default;
    constexpr Vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}

    constexpr Vec3 operator+(const Vec3& o) const { return {x + o.x, y + o.y, z + o.z}; }
    constexpr Vec3 operator-(const Vec3& o) const { return {x - o.x, y - o.y, z - o.z}; }
    constexpr Vec3 operator*(float s) const { return {x * s, y * s, z * s}; }
};

struct Quat {
    float x = 0.f, y = 0.f, z = 0.f, w = 1.f;
};

struct Transform {
    Vec3 position{0.f, 0.f, 0.f};
    Quat rotation{};
    Vec3 scale{1.f, 1.f, 1.f};
};

// ---------------------------------------------------------------------------
// Simulation version — required for determinism across nodes
// ---------------------------------------------------------------------------

struct SimulationVersion {
    uint32_t major      = 0;
    uint32_t minor      = 1;
    uint32_t patch      = 0;
    uint32_t rules_hash = 0; // hash of active simulation rules
};

} // namespace mf

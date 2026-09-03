#pragma once
#include "engine/kernel/world_clock.hpp"
#include "engine/substrate/snapshot.hpp"
#include <cstdint>
#include <cstring>
namespace mf {
using StateHash = std::uint64_t;
inline StateHash fnv1a64(StateHash h, const void* data, std::size_t n) {
    const auto* p = static_cast<const unsigned char*>(data);
    for (std::size_t i = 0; i < n; ++i) { h ^= static_cast<StateHash>(p[i]); h *= 1099511628211ull; }
    return h;
}
template <typename T>
inline StateHash fnv1a64_pod(StateHash h, const T& v) { return fnv1a64(h, &v, sizeof(v)); }
inline StateHash hash_field_snapshot(const FieldSnapshot& snap, StateHash seed = 14695981039346656037ull) {
    StateHash h = seed;
    const std::uint64_t n = snap.cells.size();
    h = fnv1a64_pod(h, n);
    for (const auto& c : snap.cells) {
        h = fnv1a64_pod(h, c.cell.x);
        h = fnv1a64_pod(h, c.cell.y);
        h = fnv1a64_pod(h, c.cell.z);
        h = fnv1a64(h, c.ch.data(), c.ch.size() * sizeof(float));
    }
    return h;
}
inline StateHash hash_world_core(const FieldSnapshot& snap, const WorldClock& clock, std::uint64_t living_entities) {
    StateHash h = hash_field_snapshot(snap);
    h = fnv1a64_pod(h, clock.tick);
    h = fnv1a64_pod(h, clock.simulation);
    const std::int64_t epoch = clock.epoch.value_or(0);
    const std::uint8_t anchored = clock.anchor.is_set() ? 1 : 0;
    h = fnv1a64_pod(h, epoch);
    h = fnv1a64_pod(h, anchored);
    h = fnv1a64_pod(h, living_entities);
    return h;
}
} // namespace mf

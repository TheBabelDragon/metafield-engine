#pragma once

#include <cstdint>
#include <functional>

namespace mf {

constexpr int CHUNK_EDGE = 8;
constexpr int CHUNK_CELLS = CHUNK_EDGE * CHUNK_EDGE * CHUNK_EDGE;

struct CellCoord {
    std::int32_t x = 0, y = 0, z = 0;
    constexpr bool operator==(const CellCoord& o) const {
        return x == o.x && y == o.y && z == o.z;
    }
};

struct ChunkCoord {
    std::int32_t x = 0, y = 0, z = 0;
    constexpr bool operator==(const ChunkCoord& o) const {
        return x == o.x && y == o.y && z == o.z;
    }
};

inline ChunkCoord chunk_of(CellCoord c) {
    auto div = [](std::int32_t v) {
        return v >= 0 ? v / CHUNK_EDGE : (v - (CHUNK_EDGE - 1)) / CHUNK_EDGE;
    };
    return {div(c.x), div(c.y), div(c.z)};
}

inline int cell_index(CellCoord c) {
    auto mod = [](std::int32_t v) {
        std::int32_t m = v % CHUNK_EDGE;
        return m < 0 ? m + CHUNK_EDGE : m;
    };
    return (mod(c.z) * CHUNK_EDGE + mod(c.y)) * CHUNK_EDGE + mod(c.x);
}

inline std::uint64_t pack_chunk(ChunkCoord c) {
    auto u = [](std::int32_t v) {
        return static_cast<std::uint64_t>(static_cast<std::uint32_t>(v));
    };
    return (u(c.x) & 0x1fffff) | ((u(c.y) & 0x1fffff) << 21) | ((u(c.z) & 0x1fffff) << 42);
}

} // namespace mf

template <>
struct std::hash<mf::ChunkCoord> {
    std::size_t operator()(const mf::ChunkCoord& c) const noexcept {
        return static_cast<std::size_t>(mf::pack_chunk(c));
    }
};

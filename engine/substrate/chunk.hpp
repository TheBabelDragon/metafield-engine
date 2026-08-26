#pragma once

#include "engine/substrate/coord.hpp"
#include "engine/substrate/channel.hpp"
#include <array>
#include <cstdint>

namespace mf {

struct CellState {
    std::array<float, CHANNEL_COUNT> ch{};
    float get(Channel c) const { return ch[static_cast<int>(c)]; }
    void  set(Channel c, float v) { ch[static_cast<int>(c)] = v; }
};

struct Chunk {
    ChunkCoord coord{};
    std::array<CellState, CHUNK_CELLS> cells{};
    bool dirty = false;
    CellState&       at(int i)       { return cells[static_cast<std::size_t>(i)]; }
    const CellState& at(int i) const { return cells[static_cast<std::size_t>(i)]; }
};

} // namespace mf

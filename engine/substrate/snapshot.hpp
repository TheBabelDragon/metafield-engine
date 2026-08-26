#pragma once
#include "engine/substrate/field.hpp"
#include <array>
#include <cmath>
#include <vector>
namespace mf {
struct CellSnap { CellCoord cell{}; std::array<float, CHANNEL_COUNT> ch{}; };
struct FieldSnapshot { std::vector<CellSnap> cells; };
inline FieldSnapshot capture(const VoxelField& field) {
    FieldSnapshot snap;
    field.each_cell_sorted([&](CellCoord c, const CellState& cell) {
        CellSnap s; s.cell = c; s.ch = cell.ch; snap.cells.push_back(s);
    });
    return snap;
}
inline void restore(VoxelField& field, const FieldSnapshot& snap) {
    for (const auto& s : snap.cells) {
        field.mark(s.cell);
        for (int i = 0; i < CHANNEL_COUNT; ++i)
            field.write(s.cell, static_cast<Channel>(i), s.ch[static_cast<std::size_t>(i)]);
    }
}
inline bool fields_equivalent(const VoxelField& a, const VoxelField& b, float eps = 1e-5f) {
    const auto sa = capture(a), sb = capture(b);
    if (sa.cells.size() != sb.cells.size()) return false;
    for (std::size_t i = 0; i < sa.cells.size(); ++i) {
        if (!(sa.cells[i].cell == sb.cells[i].cell)) return false;
        for (int c = 0; c < CHANNEL_COUNT; ++c)
            if (std::fabs(sa.cells[i].ch[static_cast<std::size_t>(c)] - sb.cells[i].ch[static_cast<std::size_t>(c)]) > eps)
                return false;
    }
    return true;
}
} // namespace mf

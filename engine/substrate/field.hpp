#pragma once
#include "engine/substrate/chunk.hpp"
#include "engine/substrate/delta.hpp"
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <vector>
namespace mf {
class VoxelField {
public:
    CellState* get(CellCoord c) {
        auto* ch = ensure_chunk(chunk_of(c));
        return ch ? &ch->at(cell_index(c)) : nullptr;
    }
    const CellState* get(CellCoord c) const {
        auto it = chunks_.find(chunk_of(c));
        if (it == chunks_.end()) return nullptr;
        return &it->second.at(cell_index(c));
    }
    float sample(CellCoord c, Channel ch) const {
        const CellState* cell = get(c);
        return cell ? cell->get(ch) : 0.f;
    }
    bool exists(CellCoord c) const { return occupied_set_.count(pack_cell(c)) != 0; }
    void mark(CellCoord c) {
        (void)get(c);
        if (occupied_set_.insert(pack_cell(c)).second) occupied_.push_back(c);
    }
    void write(CellCoord c, Channel ch, float v) {
        mark(c);
        CellState* cell = get(c);
        if (cell) cell->set(ch, v);
    }
    void apply(const FieldDeltaList& deltas) {
        for (const auto& d : deltas.items()) {
            CellState* cell = get(d.cell);
            if (cell) cell->set(d.channel, d.new_value);
        }
    }
    template <typename Fn>
    void each_cell_sorted(Fn&& fn) const {
        auto occ = occupied_;
        std::sort(occ.begin(), occ.end(), [](const CellCoord& a, const CellCoord& b) {
            if (a.z != b.z) return a.z < b.z;
            if (a.y != b.y) return a.y < b.y;
            return a.x < b.x;
        });
        occ.erase(std::unique(occ.begin(), occ.end(), [](const CellCoord& a, const CellCoord& b){ return a == b; }), occ.end());
        for (const auto& c : occ) {
            const CellState* cell = get(c);
            if (cell) fn(c, *cell);
        }
    }
    std::size_t chunk_count() const { return chunks_.size(); }
    void reserve_box(CellCoord min, CellCoord max) {
        for (std::int32_t z = min.z; z <= max.z; ++z)
        for (std::int32_t y = min.y; y <= max.y; ++y)
        for (std::int32_t x = min.x; x <= max.x; ++x)
            mark(CellCoord{x, y, z});
    }
    float sum(Channel ch) const {
        float s = 0.f;
        each_cell_sorted([&](CellCoord, const CellState& cell) { s += cell.get(ch); });
        return s;
    }
private:
    std::unordered_map<ChunkCoord, Chunk> chunks_;
    std::vector<CellCoord> occupied_;
    std::unordered_set<std::uint64_t> occupied_set_;
    Chunk* ensure_chunk(ChunkCoord cc) {
        auto it = chunks_.find(cc);
        if (it != chunks_.end()) return &it->second;
        Chunk ch; ch.coord = cc;
        auto [ins, _] = chunks_.emplace(cc, ch);
        return &ins->second;
    }
};
class FieldView {
public:
    explicit FieldView(const VoxelField& f) : f_(f) {}
    float sample(CellCoord c, Channel ch) const { return f_.sample(c, ch); }
    bool exists(CellCoord c) const { return f_.exists(c); }
    const VoxelField& field() const { return f_; }
private:
    const VoxelField& f_;
};
} // namespace mf

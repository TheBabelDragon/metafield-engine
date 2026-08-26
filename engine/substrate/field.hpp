#pragma once

#include "engine/substrate/chunk.hpp"
#include "engine/substrate/delta.hpp"
#include <unordered_map>

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
    void write(CellCoord c, Channel ch, float v) {
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
    void each_existing(Fn&& fn) const {
        for (const auto& [_, chunk] : chunks_) {
            for (int z = 0; z < CHUNK_EDGE; ++z)
            for (int y = 0; y < CHUNK_EDGE; ++y)
            for (int x = 0; x < CHUNK_EDGE; ++x) {
                CellCoord c{
                    chunk.coord.x * CHUNK_EDGE + x,
                    chunk.coord.y * CHUNK_EDGE + y,
                    chunk.coord.z * CHUNK_EDGE + z
                };
                fn(c, chunk.at(cell_index(c)));
            }
        }
    }
    std::size_t chunk_count() const { return chunks_.size(); }
    void reserve_box(CellCoord min, CellCoord max) {
        for (std::int32_t z = min.z; z <= max.z; ++z)
        for (std::int32_t y = min.y; y <= max.y; ++y)
        for (std::int32_t x = min.x; x <= max.x; ++x)
            (void)get(CellCoord{x, y, z});
    }
private:
    std::unordered_map<ChunkCoord, Chunk> chunks_;
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
    const VoxelField& field() const { return f_; }
private:
    const VoxelField& f_;
};

} // namespace mf

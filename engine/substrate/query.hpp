#pragma once
#include "engine/substrate/tick_stream.hpp"
#include <vector>
namespace mf {
inline std::vector<FieldDelta> deltas_for_cell(const FieldTickStream& stream, CellCoord cell) {
    std::vector<FieldDelta> out;
    for (const auto& t : stream.ticks()) for (const auto& d : t.deltas.items()) if (d.cell == cell) out.push_back(d);
    return out;
}
inline std::vector<FieldDelta> deltas_for_channel_after(const FieldTickStream& stream, Channel ch, std::uint64_t after_seq) {
    std::vector<FieldDelta> out;
    for (const auto& t : stream.ticks()) {
        if (t.sequence <= after_seq) continue;
        for (const auto& d : t.deltas.items()) if (d.channel == ch) out.push_back(d);
    }
    return out;
}
inline std::vector<FieldDelta> deltas_for_system(const FieldTickStream& stream, std::uint32_t system_id) {
    std::vector<FieldDelta> out;
    for (const auto& t : stream.ticks()) for (const auto& d : t.deltas.items()) if (d.system_id == system_id) out.push_back(d);
    return out;
}
inline std::vector<FieldDelta> deltas_in_region(const FieldTickStream& stream, CellCoord min, CellCoord max) {
    std::vector<FieldDelta> out;
    for (const auto& t : stream.ticks()) for (const auto& d : t.deltas.items()) {
        if (d.cell.x < min.x || d.cell.x > max.x) continue;
        if (d.cell.y < min.y || d.cell.y > max.y) continue;
        if (d.cell.z < min.z || d.cell.z > max.z) continue;
        out.push_back(d);
    }
    return out;
}
} // namespace mf

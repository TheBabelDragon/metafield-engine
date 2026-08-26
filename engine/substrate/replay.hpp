#pragma once
#include "engine/substrate/snapshot.hpp"
#include "engine/substrate/tick_stream.hpp"
namespace mf {
inline VoxelField replay(const FieldSnapshot& snap, const std::vector<FieldTick>& ticks) {
    VoxelField field; restore(field, snap);
    for (const auto& t : ticks) field.apply(t.deltas);
    return field;
}
inline VoxelField replay(const FieldSnapshot& snap, const FieldTickStream& stream,
                         std::uint64_t from_seq, std::uint64_t to_seq) {
    return replay(snap, stream.range(from_seq, to_seq));
}
} // namespace mf

#pragma once
#include "engine/substrate/coord.hpp"
#include "engine/substrate/channel.hpp"
#include <algorithm>
#include <cstdint>
#include <vector>
namespace mf {
struct FieldDelta {
    CellCoord cell{};
    Channel channel = Channel::Energy;
    float old_value = 0.f;
    float new_value = 0.f;
    std::uint64_t tick = 0;
    std::uint32_t system_id = 0;
};
class FieldDeltaList {
public:
    void push(FieldDelta d) { if (d.old_value == d.new_value) return; items_.push_back(d); }
    void push(CellCoord cell, Channel ch, float old_v, float new_v,
              std::uint32_t system_id = 0, std::uint64_t tick = 0) {
        push(FieldDelta{cell, ch, old_v, new_v, tick, system_id});
    }
    void clear() { items_.clear(); }
    const std::vector<FieldDelta>& items() const { return items_; }
    std::vector<FieldDelta>& items() { return items_; }
    std::size_t size() const { return items_.size(); }
    void sort_deterministic() {
        std::sort(items_.begin(), items_.end(), [](const FieldDelta& a, const FieldDelta& b) {
            if (a.cell.z != b.cell.z) return a.cell.z < b.cell.z;
            if (a.cell.y != b.cell.y) return a.cell.y < b.cell.y;
            if (a.cell.x != b.cell.x) return a.cell.x < b.cell.x;
            if (a.channel != b.channel) return static_cast<int>(a.channel) < static_cast<int>(b.channel);
            return a.system_id < b.system_id;
        });
    }
    std::size_t count_channel(Channel ch) const {
        std::size_t n = 0; for (const auto& d : items_) if (d.channel == ch) ++n; return n;
    }
    const FieldDelta* find(CellCoord c, Channel ch) const {
        for (const auto& d : items_)
            if (d.cell.x == c.x && d.cell.y == c.y && d.cell.z == c.z && d.channel == ch) return &d;
        return nullptr;
    }
private:
    std::vector<FieldDelta> items_;
};
struct FieldTick {
    std::uint64_t sequence = 0;
    double time = 0.0;
    float dt = 0.f;
    FieldDeltaList deltas;
};
inline const char* system_name(std::uint32_t id) {
    switch (id) {
        case 1: return "diffusion";
        case 2: return "information_decay";
        case 3: return "advection";
        default: return "unknown";
    }
}
} // namespace mf

#pragma once

#include "engine/substrate/coord.hpp"
#include "engine/substrate/channel.hpp"
#include <vector>

namespace mf {

struct FieldDelta {
    CellCoord cell{};
    Channel   channel = Channel::Energy;
    float     old_value = 0.f;
    float     new_value = 0.f;
};

class FieldDeltaList {
public:
    void push(CellCoord cell, Channel ch, float old_v, float new_v) {
        if (old_v == new_v) return;
        items_.push_back(FieldDelta{cell, ch, old_v, new_v});
    }
    void clear() { items_.clear(); }
    const std::vector<FieldDelta>& items() const { return items_; }
    std::size_t size() const { return items_.size(); }
private:
    std::vector<FieldDelta> items_;
};

} // namespace mf

#pragma once

#include "engine/substrate/system.hpp"
#include <memory>
#include <vector>

namespace mf {

class FieldScheduler {
public:
    void add(std::unique_ptr<FieldSystem> sys) { systems_.push_back(std::move(sys)); }
    FieldDeltaList tick(VoxelField& field, float dt) {
        FieldView view(field);
        FieldDeltaList all;
        for (auto& sys : systems_) {
            FieldDeltaList local;
            sys->evaluate(view, local, dt);
            for (const auto& d : local.items())
                all.push(d.cell, d.channel, d.old_value, d.new_value);
        }
        field.apply(all);
        return all;
    }
    std::size_t system_count() const { return systems_.size(); }
private:
    std::vector<std::unique_ptr<FieldSystem>> systems_;
};

} // namespace mf

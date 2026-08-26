#pragma once
#include "engine/substrate/system.hpp"
#include "engine/substrate/tick_stream.hpp"
#include <cmath>
#include <memory>
#include <vector>
namespace mf {
class FieldScheduler {
public:
    void add(std::unique_ptr<FieldSystem> sys) { systems_.push_back(std::move(sys)); }
    void attach(FieldTickStream& stream) { stream_ = &stream; }
    FieldTick step(VoxelField& field, float dt) {
        FieldView view(field);
        FieldDeltaList all;
        for (auto& sys : systems_) {
            FieldDeltaList local;
            sys->evaluate(view, local, dt);
            for (auto d : local.items()) { d.system_id = sys->id(); all.push(d); }
        }
        FieldDeltaList committed;
        for (auto d : all.items()) {
            const float present = field.sample(d.cell, d.channel);
            if (std::fabs(present - d.old_value) > 1e-5f) continue;
            d.tick = sequence_ + 1;
            committed.push(d);
        }
        committed.sort_deterministic();
        field.apply(committed);
        time_ += static_cast<double>(dt);
        ++sequence_;
        FieldTick tick;
        tick.sequence = sequence_;
        tick.time = time_;
        tick.dt = dt;
        tick.deltas = std::move(committed);
        last_ = tick;
        if (stream_) stream_->publish(tick);
        return tick;
    }
    const FieldTick& last() const { return last_; }
    std::uint64_t sequence() const { return sequence_; }
    std::size_t system_count() const { return systems_.size(); }
private:
    std::vector<std::unique_ptr<FieldSystem>> systems_;
    FieldTickStream* stream_ = nullptr;
    std::uint64_t sequence_ = 0;
    double time_ = 0.0;
    FieldTick last_{};
};
} // namespace mf

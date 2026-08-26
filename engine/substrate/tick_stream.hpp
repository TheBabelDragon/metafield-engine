#pragma once
#include "engine/substrate/delta.hpp"
#include <vector>
namespace mf {
class FieldTickSubscriber {
public:
    virtual ~FieldTickSubscriber() = default;
    virtual void on_tick(const FieldTick& tick) = 0;
};
class FieldTickStream {
public:
    void subscribe(FieldTickSubscriber& sub) { subs_.push_back(&sub); }
    void publish(const FieldTick& tick) {
        ticks_.push_back(tick);
        for (auto* s : subs_) s->on_tick(ticks_.back());
    }
    const std::vector<FieldTick>& ticks() const { return ticks_; }
    std::size_t size() const { return ticks_.size(); }
    std::vector<FieldTick> range(std::uint64_t from_seq, std::uint64_t to_seq) const {
        std::vector<FieldTick> out;
        for (const auto& t : ticks_) {
            if (t.sequence >= from_seq && t.sequence <= to_seq) out.push_back(t);
        }
        return out;
    }
private:
    std::vector<FieldTick> ticks_;
    std::vector<FieldTickSubscriber*> subs_;
};
struct DeltaCounter : FieldTickSubscriber {
    std::size_t ticks = 0;
    std::size_t temperature = 0;
    std::size_t information = 0;
    std::size_t systems_seen = 0;
    std::size_t cells = 0;
    void on_tick(const FieldTick& tick) override {
        ++ticks;
        bool seen[4] = {false, false, false, false};
        for (const auto& d : tick.deltas.items()) {
            if (d.channel == Channel::Temperature) ++temperature;
            if (d.channel == Channel::Information) ++information;
            if (d.system_id < 4) seen[d.system_id] = true;
            ++cells;
        }
        if (seen[1]) ++systems_seen;
        if (seen[2]) ++systems_seen;
        if (seen[3]) ++systems_seen;
    }
};
} // namespace mf

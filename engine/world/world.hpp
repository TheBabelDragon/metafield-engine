#pragma once

#include "engine/core/types.hpp"
#include "engine/ecs/registry.hpp"
#include "engine/world/time.hpp"
#include "engine/fields/field.hpp"
#include "engine/kernel/world_clock.hpp"
#include "engine/kernel/world_event.hpp"
#include "engine/kernel/world_tick.hpp"
#include "engine/kernel/world_state.hpp"
#include "engine/kernel/world_history.hpp"
#include "engine/substrate/field.hpp"
#include "engine/substrate/scheduler.hpp"

#include <utility>
#include <vector>

namespace mf {

class World {
public:
    static World create(WorldId id = 1) {
        World w;
        w.world_id_ = id;
        w.clock_ = WorldClock::from_scarcity(resolve_clock());
        w.history_.genesis = w.capture_state();
        return w;
    }

    World() = default;

    TimeState& time() { return time_; }
    const TimeState& time() const { return time_; }
    WorldClock& clock() { return clock_; }
    const WorldClock& clock() const { return clock_; }
    EntityRegistry& entities() { return entities_; }
    const EntityRegistry& entities() const { return entities_; }
    FieldRegistry& fields() { return fields_; }
    const FieldRegistry& fields() const { return fields_; }
    VoxelField& substrate() { return substrate_; }
    const VoxelField& substrate() const { return substrate_; }
    FieldScheduler& simulation() { return sched_; }
    const FieldScheduler& simulation() const { return sched_; }
    WorldHistory& history() { return history_; }
    const WorldHistory& history() const { return history_; }
    SimulationVersion& version() { return version_; }
    const SimulationVersion& version() const { return version_; }

    EntityID spawn() { return entities_.spawn(); }
    void destroy(EntityID id) { entities_.destroy(id); }

    void tick(double dt_sim) {
        time_.advance(dt_sim);
        clock_.advance(dt_sim);
    }

    void push_event(WorldEvent ev) {
        ev.sequence = ++event_seq_;
        pending_.push_back(std::move(ev));
    }

    WorldTick step(float dt) {
        std::vector<WorldEvent> inputs = std::move(pending_);
        pending_.clear();
        FieldDeltaList event_deltas;
        std::vector<EntityDelta> entity_deltas;
        for (const auto& ev : inputs) {
            if (ev.type == EventType::Impulse) {
                const float old = substrate_.sample(ev.cell, ev.channel);
                const float neu = old + ev.value;
                substrate_.mark(ev.cell);
                event_deltas.push(ev.cell, ev.channel, old, neu, 0, clock_.tick + 1);
                substrate_.write(ev.cell, ev.channel, neu);
            } else if (ev.type == EventType::Spawn) {
                EntityID id = spawn();
                entity_deltas.push_back(EntityDelta{EntityOp::Spawn, id, ev.name, ev.value});
            } else if (ev.type == EventType::Destroy && ev.source != INVALID_ENTITY) {
                destroy(ev.source);
                entity_deltas.push_back(EntityDelta{EntityOp::Destroy, ev.source, ev.name, 0.f});
            }
        }
        FieldTick field_tick = sched_.step(substrate_, dt);
        tick(static_cast<double>(dt));
        WorldTick wt;
        wt.world = world_id_;
        wt.sequence = clock_.tick;
        wt.simulation_time = clock_.simulation;
        wt.dt = dt;
        wt.clock = clock_;
        wt.inputs = std::move(inputs);
        for (const auto& d : event_deltas.items()) wt.field_deltas.push(d);
        for (const auto& d : field_tick.deltas.items()) wt.field_deltas.push(d);
        wt.field_deltas.sort_deterministic();
        wt.entity_deltas = std::move(entity_deltas);
        wt.state_hash = state_hash();
        history_.append(wt);
        last_ = wt;
        return wt;
    }

    WorldState capture_state() const {
        return capture_world_state(world_id_, substrate_, clock_, entities_.living_count());
    }
    StateHash state_hash() const { return capture_state().hash; }
    FieldSample sample(FieldType type, const Vec3& position) const {
        return fields_.sample(type, position);
    }

private:
    WorldId world_id_ = 1;
    TimeState time_;
    WorldClock clock_;
    EntityRegistry entities_;
    FieldRegistry fields_;
    VoxelField substrate_;
    FieldScheduler sched_;
    WorldHistory history_;
    SimulationVersion version_;
    std::vector<WorldEvent> pending_;
    std::uint64_t event_seq_ = 0;
    WorldTick last_{};
};

inline World replay(const WorldHistory& history) {
    World w = World::create(history.genesis.world);
    w.clock() = history.genesis.clock;
    restore(w.substrate(), history.genesis.substrate);
    w.history().genesis = history.genesis;
    for (const auto& t : history.ticks) {
        w.substrate().apply(t.field_deltas);
        w.clock() = t.clock;
        w.time().simulation_time = t.simulation_time;
        w.time().tick_count = t.sequence;
        w.history().append(t);
    }
    return w;
}

} // namespace mf

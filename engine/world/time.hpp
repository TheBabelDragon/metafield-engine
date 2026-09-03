#pragma once

#include "engine/world/scarcity_clock.hpp"

#include <cstdint>

namespace mf {

// Simulation tick is process order. Scarcity is Bitcoin height/work.
// Real is observed_at only — never the World epoch.

enum class TimeDomain : uint8_t {
    Simulation,
    Real,
    Scarcity,
    Network,
    Sensor,
    Replay
};

struct TimeState {
    double simulation_time = 0.0;
    double delta_time      = 0.0;
    double real_time       = 0.0;
    float  time_scale      = 1.0f;
    bool   paused          = false;
    uint64_t tick_count    = 0;
    uint32_t fixed_dt_us   = 16667;
    ScarcityClock scarcity{};

    void refresh_scarcity() { scarcity = resolve_clock(); }

    void advance(double dt_real) {
        real_time += dt_real;
        if (paused || time_scale == 0.f) {
            delta_time = 0.0;
            return;
        }
        delta_time = dt_real * static_cast<double>(time_scale);
        simulation_time += delta_time;
        ++tick_count;
    }
};

} // namespace mf

#pragma once

#include <cstdint>

namespace mf {

// ---------------------------------------------------------------------------
// Time is a first-class subsystem.
// Multiple clocks exist because distributed + physical + replay demand it.
// ---------------------------------------------------------------------------

enum class TimeDomain : uint8_t {
    Simulation,   // authoritative simulation clock
    Real,         // wall-clock / host time
    Network,      // synchronised network time
    Sensor,       // physical sensor capture time
    Replay        // deterministic replay clock
};

struct TimeState {
    // Simulation time (fixed-point friendly later; float for now)
    double simulation_time = 0.0;
    double delta_time      = 0.0;   // last tick dt (simulation)

    // Real / wall time
    double real_time       = 0.0;

    // Control
    float  time_scale      = 1.0f;  // 1×, 10×, 0.1×, paused = 0
    bool   paused          = false;

    // Determinism helpers
    uint64_t tick_count    = 0;
    uint32_t fixed_dt_us   = 16667; // ~60 Hz default (microseconds)

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

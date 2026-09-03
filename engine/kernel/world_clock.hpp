#pragma once

#include "engine/world/scarcity_clock.hpp"

#include <cstdint>
#include <optional>
#include <string>

namespace mf {

// Simulation time belongs to the World. External anchors (Bitcoin, GPS, …)
// never own the kernel clock.

enum class AnchorKind : uint8_t {
    None = 0,
    Bitcoin,
    Gps,
    Ntp,
    LocalOscillator,
    Manual,
    Simulation
};

inline const char* anchor_kind_name(AnchorKind k) {
    switch (k) {
        case AnchorKind::Bitcoin: return "bitcoin";
        case AnchorKind::Gps: return "gps";
        case AnchorKind::Ntp: return "ntp";
        case AnchorKind::LocalOscillator: return "local";
        case AnchorKind::Manual: return "manual";
        case AnchorKind::Simulation: return "simulation";
        default: return "none";
    }
}

struct ExternalClockAnchor {
    AnchorKind kind = AnchorKind::None;
    std::optional<std::int64_t> epoch;
    std::string id;
    std::string weight;
    std::string source = "none";
    ClockConfidence confidence = ClockConfidence::None;
    bool is_set() const { return kind != AnchorKind::None && epoch.has_value(); }
};

struct WorldClock {
    double simulation = 0.0;
    double dt = 0.0;
    std::uint64_t tick = 0;
    std::optional<std::int64_t> epoch;
    ExternalClockAnchor anchor;
    void advance(double dt_sim) {
        dt = dt_sim;
        if (dt_sim == 0.0) return;
        simulation += dt_sim;
        ++tick;
    }
    static WorldClock unanchored() { return {}; }
    static ExternalClockAnchor bitcoin_from(const ScarcityClock& s) {
        ExternalClockAnchor a;
        if (!s.is_anchored()) return a;
        a.kind = AnchorKind::Bitcoin;
        a.epoch = s.btc_height;
        a.id = s.btc_block_hash;
        a.weight = s.btc_work;
        a.source = s.source;
        a.confidence = s.confidence;
        return a;
    }
    static WorldClock from_scarcity(const ScarcityClock& s) {
        WorldClock c;
        c.anchor = bitcoin_from(s);
        if (c.anchor.epoch) c.epoch = c.anchor.epoch;
        return c;
    }
};

} // namespace mf

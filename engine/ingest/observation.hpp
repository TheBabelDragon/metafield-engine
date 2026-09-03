#pragma once

#include "engine/core/types.hpp"
#include "engine/world/scarcity_clock.hpp"

#include <string>
#include <vector>

namespace mf {

struct FieldRegion {
    std::string name;
    float observed   = 0.f;
    float confidence = 1.f;
};

struct FieldObservation {
    std::string body_id;
    std::string body_type = "wifi_csi";
    std::string timestamp;
    ScarcityClock clock;
    std::vector<FieldRegion> regions;
    std::vector<float> csi;
    float rssi_dbm = -90.f;
    bool  synthetic = false;
    bool  valid = false;

    float region(const char* name, float fallback = 0.f) const {
        for (const auto& r : regions) {
            if (r.name == name) return r.observed;
        }
        return fallback;
    }
};

} // namespace mf

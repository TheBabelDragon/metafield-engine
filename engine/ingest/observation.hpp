#pragma once
#include "engine/core/types.hpp"
#include "engine/kernel/provenance.hpp"
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
    SourceClass source_class = SourceClass::Unknown;
    std::string instrument;
    float region(const char* name, float fallback = 0.f) const {
        for (const auto& r : regions) if (r.name == name) return r.observed;
        return fallback;
    }
    Provenance provenance() const {
        Provenance p;
        p.cls = source_class;
        if (p.cls == SourceClass::Unknown)
            p.cls = synthetic ? SourceClass::Synthetic : SourceClass::Physical;
        p.source_id = body_id;
        p.instrument = instrument.empty() ? body_type : instrument;
        return p;
    }
};
} // namespace mf

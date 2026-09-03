#pragma once
#include <cstdlib>
#include <cstdint>
#include <string>
#include <string_view>
namespace mf {
enum class SourceClass : std::uint8_t { Unknown = 0, Physical, Derived, Synthetic };
inline const char* source_class_name(SourceClass c) {
    switch (c) {
        case SourceClass::Physical: return "physical";
        case SourceClass::Derived: return "derived";
        case SourceClass::Synthetic: return "synthetic";
        default: return "unknown";
    }
}
inline SourceClass source_class_from(std::string_view s) {
    if (s == "physical" || s == "sensor" || s == "instrument") return SourceClass::Physical;
    if (s == "derived" || s == "inferred" || s == "interpolated") return SourceClass::Derived;
    if (s == "synthetic" || s == "simulated" || s == "model") return SourceClass::Synthetic;
    return SourceClass::Unknown;
}
inline bool synthetic_allowed() {
    const char* e = std::getenv("METAFIELD_ALLOW_SYNTHETIC");
    return e && e[0] && e[0] != '0';
}
struct Provenance {
    SourceClass cls = SourceClass::Unknown;
    std::string source_id;
    std::string instrument;
    float uncertainty = 0.f;
    bool is_synthetic() const { return cls == SourceClass::Synthetic || cls == SourceClass::Unknown; }
    bool is_physical() const { return cls == SourceClass::Physical; }
    static Provenance physical(std::string id, std::string instrument = {}) {
        return Provenance{SourceClass::Physical, std::move(id), std::move(instrument), 0.f};
    }
    static Provenance derived(std::string id) {
        return Provenance{SourceClass::Derived, std::move(id), {}, 0.f};
    }
    static Provenance synthetic(std::string id) {
        return Provenance{SourceClass::Synthetic, std::move(id), {}, 0.f};
    }
};
} // namespace mf

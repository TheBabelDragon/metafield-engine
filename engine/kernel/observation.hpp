#pragma once
#include "engine/kernel/provenance.hpp"
#include "engine/kernel/quantity.hpp"
#include "engine/kernel/world_clock.hpp"
#include "engine/substrate/coord.hpp"
#include <optional>
#include <string>
namespace mf {
struct Observation {
    Quantity quantity = Quantity::Unknown;
    double value = 0.0;
    const char* unit = "";
    double uncertainty = 0.0;
    Provenance provenance;
    std::optional<CellCoord> where;
    WorldClock when;
    std::string note;
    bool admissible(bool allow_synthetic) const {
        if (quantity == Quantity::Unknown) return false;
        if (provenance.is_synthetic() && !allow_synthetic) return false;
        return true;
    }
};
} // namespace mf

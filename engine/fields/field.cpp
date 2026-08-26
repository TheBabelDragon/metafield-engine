#include "engine/fields/field.hpp"
#include <cmath>

namespace mf {

// ---------------------------------------------------------------------------
// Placeholder analytic fields — replace with real grids / hardware later
// ---------------------------------------------------------------------------

void OpticalField::sample(const Vec3& position, FieldSample& out) const {
    // Simple radial falloff from origin for smoke-testing
    float r = std::sqrt(position.x * position.x +
                        position.y * position.y +
                        position.z * position.z);
    out.scalar     = std::exp(-r * 0.1f);
    out.intensity  = out.scalar;
    out.confidence = 1.0f;
    out.valid      = true;
}

void ThermalField::sample(const Vec3& position, FieldSample& out) const {
    // Constant ambient + tiny gradient
    out.scalar     = 24.0f + position.y * 0.01f; // °C-ish
    out.intensity  = out.scalar;
    out.confidence = 1.0f;
    out.valid      = true;
}

void AcousticField::sample(const Vec3& position, FieldSample& out) const {
    // Quiet ambient
    out.scalar     = 0.05f;
    out.intensity  = out.scalar;
    out.confidence = 1.0f;
    out.valid      = true;
    (void)position;
}

} // namespace mf

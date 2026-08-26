#include "engine/fields/csi_field.hpp"

namespace mf {

void CsiField::sample(const Vec3& /*position*/, FieldSample& out) const {
    std::lock_guard<std::mutex> lock(mu_);
    if (!latest_.valid) {
        out = FieldSample{};
        return;
    }
    out.scalar     = latest_.region("csi_energy");
    out.intensity  = latest_.region("csi_energy");
    out.vector     = Vec3{latest_.region("rssi"),
                          latest_.region("csi_mean"),
                          latest_.region("csi_spread")};
    out.confidence = latest_.synthetic ? 0.4f : 0.95f;
    out.valid      = true;
}

void CsiField::ingest(const FieldObservation& obs) {
    if (!obs.valid) return;
    std::lock_guard<std::mutex> lock(mu_);
    auto& body = bodies_[obs.body_id];
    body.last = obs;
    ++body.packet_count;
    latest_ = obs;
    ++packets_;
}

std::size_t CsiField::body_count() const {
    std::lock_guard<std::mutex> lock(mu_);
    return bodies_.size();
}

FieldObservation CsiField::latest() const {
    std::lock_guard<std::mutex> lock(mu_);
    return latest_;
}

std::unordered_map<std::string, CsiBodyState> CsiField::bodies() const {
    std::lock_guard<std::mutex> lock(mu_);
    return bodies_;
}

} // namespace mf

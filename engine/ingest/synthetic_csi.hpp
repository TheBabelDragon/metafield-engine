#pragma once
#include "engine/ingest/observation.hpp"
#include <cmath>
#include <string>
#include <cstdint>
namespace mf {
inline FieldObservation make_synthetic_csi(std::uint64_t tick, const char* body = "synthetic_cyd") {
    FieldObservation o;
    o.body_id = body;
    o.body_type = "wifi_csi";
    o.timestamp = std::to_string(tick);
    o.synthetic = true;
    o.source_class = SourceClass::Synthetic;
    o.instrument = "synthetic_csi";
    o.valid = true;
    const float t = static_cast<float>(tick) * 0.08f;
    o.rssi_dbm = -58.f + 8.f * std::sin(t * 0.35f);
    o.csi.resize(32);
    for (int i = 0; i < 32; ++i) {
        const float k = static_cast<float>(i) / 31.f;
        o.csi[i] = 0.35f + 0.25f * std::sin(t + k * 6.28f)
                 + 0.12f * std::sin(t * 2.2f + k * 12.f)
                 + 0.05f * std::sin(t * 7.0f + static_cast<float>(i));
        if (o.csi[i] < 0.f) o.csi[i] = 0.f;
        if (o.csi[i] > 1.f) o.csi[i] = 1.f;
    }
    float sum = 0.f, sq = 0.f, peak = 0.f;
    for (float v : o.csi) { sum += v; sq += v * v; if (v > peak) peak = v; }
    const float n = 32.f;
    const float mean = sum / n;
    const float energy = std::sqrt(sq / n);
    float var = 0.f;
    for (float v : o.csi) { float d = v - mean; var += d * d; }
    const float spread = std::sqrt(var / n);
    const float rssi_n = std::max(0.f, std::min(1.f, (o.rssi_dbm + 90.f) / 60.f));
    o.regions = {
        {"rssi", rssi_n, 0.4f},
        {"csi_mean", mean, 0.4f},
        {"csi_peak", peak, 0.4f},
        {"csi_energy", energy, 0.4f},
        {"csi_spread", std::min(1.f, spread * 2.f), 0.4f},
    };
    return o;
}
} // namespace mf

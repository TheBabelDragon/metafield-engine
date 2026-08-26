#pragma once

#include "engine/ingest/observation.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <deque>
#include <string>
#include <vector>

namespace mf {

struct OccupancyBlob {
    float x = 0.f;
    float z = 0.f;
    float rx = 0.6f;
    float rz = 0.4f;
    float angle = 0.f;
    float energy = 0.f;
    float motion = 0.f;
    std::vector<float> contour;
};

struct CsiEstimate {
    int gw = 24;
    int gz = 24;
    std::vector<float> grid;
    std::vector<OccupancyBlob> blobs;
    float motion = 0.f;
    float energy = 0.f;
    bool live = false;
};

class CsiInferencer {
public:
    void push(const FieldObservation& obs) {
        if (!obs.valid || obs.csi.empty()) return;
        Frame f;
        f.csi = obs.csi;
        f.rssi = obs.region("rssi");
        f.energy = obs.region("csi_energy");
        f.synthetic = obs.synthetic;
        if (hist_.size() >= kMax) hist_.pop_front();
        hist_.push_back(std::move(f));
    }

    CsiEstimate estimate() const {
        CsiEstimate e;
        e.grid.assign(static_cast<size_t>(e.gw * e.gz), 0.f);
        if (hist_.empty()) return e;
        const Frame& cur = hist_.back();
        e.live = !cur.synthetic;
        e.energy = cur.energy;
        const Frame* prev = hist_.size() > 1 ? &hist_[hist_.size() - 2] : nullptr;
        float motion_acc = 0.f;
        int motion_n = 0;
        const int n = static_cast<int>(cur.csi.size());
        for (int i = 0; i < n; ++i) {
            const float amp = std::clamp(cur.csi[static_cast<size_t>(i)], 0.f, 1.f);
            float d = 0.f;
            if (prev && i < static_cast<int>(prev->csi.size())) {
                d = std::fabs(amp - prev->csi[static_cast<size_t>(i)]);
                motion_acc += d;
                ++motion_n;
            }
            const float theta = (static_cast<float>(i) / std::max(1, n - 1) - 0.5f) * 6.2831853f;
            const float range = 0.7f + (1.f - amp) * 3.2f;
            splat(e, std::cos(theta) * range, std::sin(theta) * range, 0.22f + amp * 0.55f, 0.55f);
            if (d > 0.04f) {
                const float mr = 1.1f + (1.f - d) * 2.4f;
                splat(e, std::cos(theta) * mr, std::sin(theta) * mr, 0.18f + d * 0.7f, 0.45f);
            }
        }
        e.motion = motion_n ? motion_acc / static_cast<float>(motion_n) : 0.f;
        extract_blobs(e);
        return e;
    }

private:
    static constexpr size_t kMax = 48;
    static constexpr float kWorld = 6.f;
    struct Frame {
        std::vector<float> csi;
        float rssi = 0.f;
        float energy = 0.f;
        bool synthetic = true;
    };
    std::deque<Frame> hist_;

    static void splat(CsiEstimate& e, float x, float z, float amp, float sigma) {
        const float half = kWorld * 0.5f;
        const int gw = e.gw, gz = e.gz;
        const float sx = static_cast<float>(gw) / kWorld;
        const float sz = static_cast<float>(gz) / kWorld;
        const int cx = static_cast<int>((x + half) * sx);
        const int cz = static_cast<int>((z + half) * sz);
        const int rad = std::max(1, static_cast<int>(sigma * sx * 2.f));
        const float inv = 1.f / (2.f * sigma * sigma);
        for (int jz = cz - rad; jz <= cz + rad; ++jz) {
            if (jz < 0 || jz >= gz) continue;
            for (int ix = cx - rad; ix <= cx + rad; ++ix) {
                if (ix < 0 || ix >= gw) continue;
                const float px = (static_cast<float>(ix) + 0.5f) / sx - half;
                const float pz = (static_cast<float>(jz) + 0.5f) / sz - half;
                const float dx = px - x, dz = pz - z;
                const float w = amp * std::exp(-(dx * dx + dz * dz) * inv);
                e.grid[static_cast<size_t>(jz * gw + ix)] += w;
            }
        }
    }

    static void extract_blobs(CsiEstimate& e) {
        float peak = 0.f;
        for (float v : e.grid) peak = std::max(peak, v);
        if (peak < 1e-4f) return;
        const float thr = peak * 0.42f;
        const int gw = e.gw, gz = e.gz;
        std::vector<char> used(e.grid.size(), 0);
        auto idx = [&](int x, int z) { return z * gw + x; };
        for (int seed = 0; seed < 3; ++seed) {
            int bx = -1, bz = -1;
            float best = thr;
            for (int z = 1; z < gz - 1; ++z) {
                for (int x = 1; x < gw - 1; ++x) {
                    if (used[static_cast<size_t>(idx(x, z))]) continue;
                    const float v = e.grid[static_cast<size_t>(idx(x, z))];
                    if (v > best) { best = v; bx = x; bz = z; }
                }
            }
            if (bx < 0) break;
            float sx = 0, sz = 0, sw = 0, sxx = 0, szz = 0, sxz = 0;
            std::vector<std::pair<int,int>> q{{bx,bz}};
            used[static_cast<size_t>(idx(bx, bz))] = 1;
            for (size_t qi = 0; qi < q.size(); ++qi) {
                auto [x, z] = q[qi];
                const float w = e.grid[static_cast<size_t>(idx(x, z))];
                const float px = (static_cast<float>(x) + 0.5f) / static_cast<float>(gw) * kWorld - kWorld * 0.5f;
                const float pz = (static_cast<float>(z) + 0.5f) / static_cast<float>(gz) * kWorld - kWorld * 0.5f;
                sx += px * w; sz += pz * w; sw += w;
                sxx += px * px * w; szz += pz * pz * w; sxz += px * pz * w;
                for (int dz = -1; dz <= 1; ++dz) {
                    for (int dx = -1; dx <= 1; ++dx) {
                        if (!dx && !dz) continue;
                        int nx = x + dx, nz = z + dz;
                        if (nx < 0 || nz < 0 || nx >= gw || nz >= gz) continue;
                        size_t ii = static_cast<size_t>(idx(nx, nz));
                        if (used[ii] || e.grid[ii] < thr) continue;
                        used[ii] = 1;
                        q.push_back({nx, nz});
                    }
                }
            }
            if (sw < 1e-4f) continue;
            OccupancyBlob b;
            b.x = sx / sw;
            b.z = sz / sw;
            b.energy = std::min(1.f, best / std::max(peak, 1e-4f));
            b.motion = e.motion;
            const float varx = std::max(0.05f, sxx / sw - b.x * b.x);
            const float varz = std::max(0.05f, szz / sw - b.z * b.z);
            const float cov  = sxz / sw - b.x * b.z;
            b.rx = std::clamp(std::sqrt(varx) * 2.1f, 0.25f, 2.2f);
            b.rz = std::clamp(std::sqrt(varz) * 2.1f, 0.25f, 2.2f);
            b.angle = 0.5f * std::atan2(2.f * cov, varx - varz);
            b.contour.reserve(36);
            for (int i = 0; i < 18; ++i) {
                const float t = static_cast<float>(i) / 18.f * 6.2831853f;
                const float ct = std::cos(t), st = std::sin(t);
                const float ca = std::cos(b.angle), sa = std::sin(b.angle);
                const float lx = ct * b.rx, lz = st * b.rz;
                b.contour.push_back(b.x + ca * lx - sa * lz);
                b.contour.push_back(b.z + sa * lx + ca * lz);
            }
            e.blobs.push_back(std::move(b));
        }
    }
};

} // namespace mf

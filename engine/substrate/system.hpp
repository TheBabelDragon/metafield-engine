#pragma once
#include "engine/substrate/field.hpp"
#include "engine/ingest/observation.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace mf {

class FieldSystem {
public:
    virtual ~FieldSystem() = default;
    virtual const char* name() const = 0;
    virtual std::uint32_t id() const = 0;
    virtual void evaluate(const FieldView& field, FieldDeltaList& out, float dt) = 0;
};

enum class Boundary { Open, Closed };

class DiffusionSystem : public FieldSystem {
public:
    explicit DiffusionSystem(float rate = 0.35f, Boundary bound = Boundary::Open,
                             Channel ch = Channel::Temperature)
        : rate_(rate), bound_(bound), ch_(ch) {}
    const char* name() const override { return "diffusion"; }
    std::uint32_t id() const override { return SYS_DIFFUSION; }
    void evaluate(const FieldView& field, FieldDeltaList& out, float dt) override {
        const float k = rate_ * dt;
        field.field().each_cell_sorted([&](CellCoord c, const CellState& cell) {
            const float self = cell.get(ch_);
            auto nb = [&](CellCoord n) {
                if (bound_ == Boundary::Closed && !field.exists(n)) return self;
                return field.sample(n, ch_);
            };
            const float avg = (nb({c.x-1,c.y,c.z})+nb({c.x+1,c.y,c.z})+nb({c.x,c.y-1,c.z})+nb({c.x,c.y+1,c.z})+nb({c.x,c.y,c.z-1})+nb({c.x,c.y,c.z+1})) / 6.f;
            const float next = self + (avg - self) * k;
            if (next != self) out.push(c, ch_, self, next, id());
        });
    }
private:
    float rate_; Boundary bound_; Channel ch_;
};

class InformationDecaySystem : public FieldSystem {
public:
    explicit InformationDecaySystem(float lambda = 0.15f) : lambda_(lambda) {}
    const char* name() const override { return "information.decay"; }
    std::uint32_t id() const override { return SYS_INFO_DECAY; }
    void evaluate(const FieldView& field, FieldDeltaList& out, float dt) override {
        const float factor = std::exp(-lambda_ * dt);
        field.field().each_cell_sorted([&](CellCoord c, const CellState& cell) {
            const float self = cell.get(Channel::Information);
            if (self == 0.f) return;
            float next = self * factor;
            if (next > self) next = self;
            if (next != self) out.push(c, Channel::Information, self, next, id());
        });
    }
private:
    float lambda_;
};

class DecaySystem : public FieldSystem {
public:
    explicit DecaySystem(Channel ch = Channel::Energy, float lambda = 0.4f)
        : ch_(ch), lambda_(lambda) {}
    const char* name() const override { return "decay"; }
    std::uint32_t id() const override { return SYS_DECAY; }
    void evaluate(const FieldView& field, FieldDeltaList& out, float dt) override {
        const float factor = std::exp(-lambda_ * dt);
        field.field().each_cell_sorted([&](CellCoord c, const CellState& cell) {
            const float self = cell.get(ch_);
            if (self == 0.f) return;
            float next = self * factor;
            if (next > self) next = self;
            if (next != self) out.push(c, ch_, self, next, id());
        });
    }
private:
    Channel ch_; float lambda_;
};

class AdvectionSystem : public FieldSystem {
public:
    explicit AdvectionSystem(Channel scalar = Channel::Temperature) : scalar_(scalar) {}
    const char* name() const override { return "advection"; }
    std::uint32_t id() const override { return SYS_ADVECTION; }
    void evaluate(const FieldView& field, FieldDeltaList& out, float dt) override {
        auto flux = [&](CellCoord a, CellCoord b, Channel mom) {
            if (!field.exists(a) || !field.exists(b)) return 0.f;
            const float u = 0.5f * (field.sample(a, mom) + field.sample(b, mom));
            const float Ta = field.sample(a, scalar_);
            const float Tb = field.sample(b, scalar_);
            return u >= 0.f ? u * Ta : u * Tb;
        };
        field.field().each_cell_sorted([&](CellCoord c, const CellState& cell) {
            const float T = cell.get(scalar_);
            const float div =
                (flux(c,{c.x+1,c.y,c.z},Channel::MomentumX) - flux({c.x-1,c.y,c.z},c,Channel::MomentumX)) +
                (flux(c,{c.x,c.y+1,c.z},Channel::MomentumY) - flux({c.x,c.y-1,c.z},c,Channel::MomentumY)) +
                (flux(c,{c.x,c.y,c.z+1},Channel::MomentumZ) - flux({c.x,c.y,c.z-1},c,Channel::MomentumZ));
            const float next = T - dt * div;
            if (next != T) out.push(c, scalar_, T, next, id());
        });
    }
private:
    Channel scalar_;
};

class CsiInjectSystem : public FieldSystem {
public:
    static constexpr int N = 32;
    const char* name() const override { return "csi.input"; }
    std::uint32_t id() const override { return SYS_CSI_INPUT; }
    void bind(const FieldObservation& obs) { obs_ = obs; has_ = obs.valid; }
    void clear() { has_ = false; obs_ = {}; }
    void evaluate(const FieldView& field, FieldDeltaList& out, float dt) override {
        (void)dt;
        if (!has_ || !obs_.valid) return;
        float amp = obs_.region("csi_energy");
        if (amp <= 0.f && !obs_.csi.empty()) {
            float s = 0.f; for (float v : obs_.csi) s += v;
            amp = s / static_cast<float>(obs_.csi.size());
        }
        if (amp <= 1e-6f) return;
        int peak = 0; float pv = 0.f;
        for (std::size_t i = 0; i < obs_.csi.size(); ++i)
            if (obs_.csi[i] > pv) { pv = obs_.csi[i]; peak = static_cast<int>(i); }
        int cx = 16, cz = 16;
        if (!obs_.csi.empty()) {
            const int denom = std::max(1, static_cast<int>(obs_.csi.size()) - 1);
            cx = std::clamp((peak * (N - 1)) / denom, 0, N - 1);
            cz = std::clamp(static_cast<int>(obs_.region("csi_spread") * (N - 1)), 0, N - 1);
        }
        for (int dz = -2; dz <= 2; ++dz)
        for (int dx = -2; dx <= 2; ++dx) {
            CellCoord c{cx + dx, 0, cz + dz};
            if (c.x < 0 || c.x >= N || c.z < 0 || c.z >= N) continue;
            if (!field.exists(c)) continue;
            const float w = amp * std::exp(-0.45f * float(dx * dx + dz * dz));
            const float old = field.sample(c, Channel::Energy);
            float neu = old + w * (1.f - old);
            if (neu > 1.f) neu = 1.f;
            if (neu != old) out.push(c, Channel::Energy, old, neu, id());
        }
    }
private:
    FieldObservation obs_{};
    bool has_ = false;
};

} // namespace mf

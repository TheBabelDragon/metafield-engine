#pragma once
#include "engine/substrate/field.hpp"
#include <cmath>
#include <cstdint>
namespace mf {
constexpr std::uint32_t SYS_DIFFUSION = 1;
constexpr std::uint32_t SYS_INFO_DECAY = 2;
constexpr std::uint32_t SYS_ADVECTION = 3;
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
    explicit DiffusionSystem(float rate = 0.35f, Boundary bound = Boundary::Open) : rate_(rate), bound_(bound) {}
    const char* name() const override { return "diffusion.temperature"; }
    std::uint32_t id() const override { return SYS_DIFFUSION; }
    void evaluate(const FieldView& field, FieldDeltaList& out, float dt) override {
        const float k = rate_ * dt;
        field.field().each_cell_sorted([&](CellCoord c, const CellState& cell) {
            const float self = cell.get(Channel::Temperature);
            auto nb = [&](CellCoord n) {
                if (bound_ == Boundary::Closed && !field.exists(n)) return self;
                return field.sample(n, Channel::Temperature);
            };
            const float avg = (nb({c.x-1,c.y,c.z})+nb({c.x+1,c.y,c.z})+nb({c.x,c.y-1,c.z})+nb({c.x,c.y+1,c.z})+nb({c.x,c.y,c.z-1})+nb({c.x,c.y,c.z+1})) / 6.f;
            const float next = self + (avg - self) * k;
            if (next != self) out.push(c, Channel::Temperature, self, next, id());
        });
    }
private:
    float rate_; Boundary bound_;
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
class AdvectionSystem : public FieldSystem {
public:
    explicit AdvectionSystem(Channel scalar = Channel::Temperature) : scalar_(scalar) {}
    const char* name() const override { return "advection.temperature"; }
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
} // namespace mf

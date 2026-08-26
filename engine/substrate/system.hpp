#pragma once
#include "engine/substrate/field.hpp"
#include <cmath>
#include <cstdint>
namespace mf {
constexpr std::uint32_t SYS_DIFFUSION = 1;
constexpr std::uint32_t SYS_INFO_DECAY = 2;
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
} // namespace mf

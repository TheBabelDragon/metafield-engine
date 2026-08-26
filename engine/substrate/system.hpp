#pragma once

#include "engine/substrate/field.hpp"

namespace mf {

class FieldSystem {
public:
    virtual ~FieldSystem() = default;
    virtual const char* name() const = 0;
    virtual void evaluate(const FieldView& field, FieldDeltaList& out, float dt) = 0;
};

class DiffusionSystem : public FieldSystem {
public:
    explicit DiffusionSystem(float rate = 0.35f) : rate_(rate) {}
    const char* name() const override { return "diffusion.temperature"; }
    void evaluate(const FieldView& field, FieldDeltaList& out, float dt) override {
        const float k = rate_ * dt;
        field.field().each_existing([&](CellCoord c, const CellState& cell) {
            const float self = cell.get(Channel::Temperature);
            const float n =
                field.sample({c.x - 1, c.y, c.z}, Channel::Temperature) +
                field.sample({c.x + 1, c.y, c.z}, Channel::Temperature) +
                field.sample({c.x, c.y - 1, c.z}, Channel::Temperature) +
                field.sample({c.x, c.y + 1, c.z}, Channel::Temperature) +
                field.sample({c.x, c.y, c.z - 1}, Channel::Temperature) +
                field.sample({c.x, c.y, c.z + 1}, Channel::Temperature);
            const float next = self + (n / 6.f - self) * k;
            if (next != self) out.push(c, Channel::Temperature, self, next);
        });
    }
private:
    float rate_;
};

} // namespace mf

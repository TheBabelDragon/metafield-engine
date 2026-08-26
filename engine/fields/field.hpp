#pragma once

#include "engine/core/types.hpp"
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

namespace mf {

// ---------------------------------------------------------------------------
// Field is the distinctive MetaField abstraction.
// An entity can query continuous spatial fields at any position.
// ---------------------------------------------------------------------------

enum class FieldType : uint32_t {
    Optical        = 1,
    Thermal        = 2,
    Acoustic       = 3,
    Electromagnetic = 4,
    Gravity        = 5,
    Fluid          = 6,
    Custom         = 1000
};

struct FieldSample {
    // Generic payload — concrete fields specialise meaning
    float  scalar     = 0.f;
    Vec3   vector     {0.f, 0.f, 0.f};
    float  intensity  = 0.f;
    float  confidence = 1.f;   // 0..1 — useful for real vs simulated
    bool   valid      = false;
};

class Field {
public:
    virtual ~Field() = default;

    virtual FieldType   type() const = 0;
    virtual FieldID     id()   const = 0;
    virtual std::string name() const = 0;

    // Sample the field at a world-space position.
    // Implementations may be analytic, grid-based, GPU, or live hardware.
    virtual void sample(const Vec3& position, FieldSample& out) const = 0;

    // Optional: sample a volume / ray later
};

// ---------------------------------------------------------------------------
// Concrete field stubs — real implementations live in plugins or later systems
// ---------------------------------------------------------------------------

class OpticalField : public Field {
public:
    FieldType   type() const override { return FieldType::Optical; }
    FieldID     id()   const override { return id_; }
    std::string name() const override { return "OpticalField"; }

    void sample(const Vec3& position, FieldSample& out) const override;

    explicit OpticalField(FieldID id) : id_(id) {}

private:
    FieldID id_;
};

class ThermalField : public Field {
public:
    FieldType   type() const override { return FieldType::Thermal; }
    FieldID     id()   const override { return id_; }
    std::string name() const override { return "ThermalField"; }

    void sample(const Vec3& position, FieldSample& out) const override;

    explicit ThermalField(FieldID id) : id_(id) {}

private:
    FieldID id_;
};

class AcousticField : public Field {
public:
    FieldType   type() const override { return FieldType::Acoustic; }
    FieldID     id()   const override { return id_; }
    std::string name() const override { return "AcousticField"; }

    void sample(const Vec3& position, FieldSample& out) const override;

    explicit AcousticField(FieldID id) : id_(id) {}

private:
    FieldID id_;
};

// ---------------------------------------------------------------------------
// Field Registry — owns all fields present in the world
// ---------------------------------------------------------------------------

class FieldRegistry {
public:
    template <typename T, typename... Args>
    T& create(Args&&... args) {
        auto field = std::make_unique<T>(std::forward<Args>(args)...);
        FieldID id = field->id();
        T& ref = *field;
        fields_[id] = std::move(field);
        return ref;
    }

    Field* get(FieldID id) {
        auto it = fields_.find(id);
        return it != fields_.end() ? it->second.get() : nullptr;
    }

    const Field* get(FieldID id) const {
        auto it = fields_.find(id);
        return it != fields_.end() ? it->second.get() : nullptr;
    }

    // Convenience typed sample
    template <typename FieldT>
    FieldSample sample(const Vec3& position) const {
        FieldSample out;
        for (const auto& [_, f] : fields_) {
            if (f->type() == FieldT{0}.type()) { // type match via temporary
                f->sample(position, out);
                return out;
            }
        }
        return out; // invalid
    }

    // Better: sample by type enum
    FieldSample sample(FieldType type, const Vec3& position) const {
        FieldSample out;
        for (const auto& [_, f] : fields_) {
            if (f->type() == type) {
                f->sample(position, out);
                return out;
            }
        }
        return out;
    }

    size_t count() const { return fields_.size(); }

private:
    std::unordered_map<FieldID, std::unique_ptr<Field>> fields_;
};

} // namespace mf

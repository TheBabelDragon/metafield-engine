#pragma once

#include "engine/fields/field.hpp"
#include "engine/ingest/observation.hpp"

#include <unordered_map>
#include <string>
#include <mutex>
#include <cstdint>

namespace mf {

struct CsiBodyState {
    FieldObservation last;
    std::uint64_t packet_count = 0;
};

class CsiField : public Field {
public:
    explicit CsiField(FieldID id) : id_(id) {}

    FieldType   type() const override { return FieldType::Electromagnetic; }
    FieldID     id()   const override { return id_; }
    std::string name() const override { return "CsiField"; }

    void sample(const Vec3& position, FieldSample& out) const override;
    void ingest(const FieldObservation& obs);

    std::size_t body_count() const;
    std::uint64_t packet_count() const { return packets_; }
    bool has_live() const { return packets_ > 0; }

    FieldObservation latest() const;
    std::unordered_map<std::string, CsiBodyState> bodies() const;

private:
    FieldID id_;
    mutable std::mutex mu_;
    std::unordered_map<std::string, CsiBodyState> bodies_;
    FieldObservation latest_;
    std::uint64_t packets_ = 0;
};

} // namespace mf

#pragma once

#include "engine/core/types.hpp"
#include <string>
#include <cstdint>

namespace mf {

struct Name {
    std::string value;
};

struct Renderable {
    std::string kind = "box";
    float sx = 0.4f;
    float sy = 0.8f;
    float sz = 0.4f;
    std::uint32_t color = 0x88a0b8;
};

struct SensorTag {
    std::string body_id;
};

} // namespace mf

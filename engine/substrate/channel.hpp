#pragma once

#include <cstdint>
#include <string_view>

namespace mf {

enum class Channel : std::uint8_t {
    Matter = 0,
    Energy,
    Temperature,
    Pressure,
    Charge,
    MomentumX,
    MomentumY,
    MomentumZ,
    Entropy,
    Information,
    COUNT
};

constexpr int CHANNEL_COUNT = static_cast<int>(Channel::COUNT);

inline std::string_view channel_name(Channel c) {
    switch (c) {
        case Channel::Matter:      return "matter";
        case Channel::Energy:      return "energy";
        case Channel::Temperature: return "temperature";
        case Channel::Pressure:    return "pressure";
        case Channel::Charge:      return "charge";
        case Channel::MomentumX:   return "momentum_x";
        case Channel::MomentumY:   return "momentum_y";
        case Channel::MomentumZ:   return "momentum_z";
        case Channel::Entropy:     return "entropy";
        case Channel::Information: return "information";
        default:                   return "unknown";
    }
}

} // namespace mf

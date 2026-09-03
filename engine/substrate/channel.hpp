#pragma once
#include <cstdint>
#include <string_view>
namespace mf {
enum class Channel : std::uint8_t {
    Matter = 0, Energy, Temperature, Pressure, Charge,
    MomentumX, MomentumY, MomentumZ, Entropy, Information, COUNT
};
constexpr int CHANNEL_COUNT = static_cast<int>(Channel::COUNT);
inline std::string_view channel_name(Channel c) {
    switch (c) {
        case Channel::Matter: return "matter";
        case Channel::Energy: return "energy";
        case Channel::Temperature: return "temperature";
        case Channel::Pressure: return "pressure";
        case Channel::Charge: return "charge";
        case Channel::MomentumX: return "momentum_x";
        case Channel::MomentumY: return "momentum_y";
        case Channel::MomentumZ: return "momentum_z";
        case Channel::Entropy: return "entropy";
        case Channel::Information: return "information";
        default: return "unknown";
    }
}
struct ChannelSchema {
    Channel id{};
    const char* name = "";
    const char* units = "";
    bool vector = false;
};
inline ChannelSchema channel_schema(Channel c) {
    switch (c) {
        case Channel::Matter: return {c, "matter", "1", false};
        case Channel::Energy: return {c, "energy", "1", false};
        case Channel::Temperature: return {c, "temperature", "K", false};
        case Channel::Pressure: return {c, "pressure", "Pa", false};
        case Channel::Charge: return {c, "charge", "C", false};
        case Channel::MomentumX: return {c, "momentum_x", "1", true};
        case Channel::MomentumY: return {c, "momentum_y", "1", true};
        case Channel::MomentumZ: return {c, "momentum_z", "1", true};
        case Channel::Entropy: return {c, "entropy", "1", false};
        case Channel::Information: return {c, "information", "1", false};
        default: return {c, "unknown", "", false};
    }
}
} // namespace mf

#pragma once
#include "engine/substrate/delta.hpp"
#include "engine/ingest/json_lite.hpp"
#include <sstream>
#include <string>
#include <string_view>
namespace mf {
inline Channel channel_from_name(std::string_view n) {
    if (n == "energy") return Channel::Energy;
    if (n == "temperature") return Channel::Temperature;
    if (n == "information") return Channel::Information;
    if (n == "momentum_x") return Channel::MomentumX;
    if (n == "momentum_y") return Channel::MomentumY;
    if (n == "momentum_z") return Channel::MomentumZ;
    if (n == "matter") return Channel::Matter;
    return Channel::Energy;
}
inline std::string tick_to_json(const FieldTick& tick) {
    std::ostringstream os;
    os << "{\"sequence\":" << tick.sequence << ",\"time\":" << tick.time
       << ",\"dt\":" << tick.dt << ",\"deltas\":[";
    bool first = true;
    for (const auto& d : tick.deltas.items()) {
        if (!first) os << ',';
        first = false;
        os << "{\"source\":\"" << system_name(d.system_id) << "\""
           << ",\"field\":\"" << channel_name(d.channel) << "\""
           << ",\"cell\":{\"x\":" << d.cell.x << ",\"y\":" << d.cell.y << ",\"z\":" << d.cell.z << "}"
           << ",\"old_value\":" << d.old_value << ",\"new_value\":" << d.new_value
           << ",\"delta\":" << (d.new_value - d.old_value)
           << ",\"system_id\":" << d.system_id
           << ",\"tick\":" << d.tick << "}";
    }
    os << "]}";
    return os.str();
}
inline bool parse_tick_json(std::string_view line, FieldTick& tick) {
    auto seq = json_lite::get_number(line, "sequence");
    if (!seq) return false;
    tick = {};
    tick.sequence = static_cast<std::uint64_t>(*seq);
    tick.time = json_lite::get_number(line, "time").value_or(0.0);
    tick.dt = static_cast<float>(json_lite::get_number(line, "dt").value_or(0.0));
    size_t pos = 0;
    while (true) {
        auto found = line.find("\"cell\"", pos);
        if (found == std::string_view::npos) break;
        auto sl = line.substr(found);
        FieldDelta d;
        d.cell.x = static_cast<std::int32_t>(json_lite::get_number(sl, "x").value_or(0));
        d.cell.y = static_cast<std::int32_t>(json_lite::get_number(sl, "y").value_or(0));
        d.cell.z = static_cast<std::int32_t>(json_lite::get_number(sl, "z").value_or(0));
        if (auto ch = json_lite::get_string(sl, "field")) d.channel = channel_from_name(*ch);
        else if (auto ch2 = json_lite::get_string(sl, "channel")) d.channel = channel_from_name(*ch2);
        d.old_value = static_cast<float>(json_lite::get_number(sl, "old_value").value_or(0));
        d.new_value = static_cast<float>(json_lite::get_number(sl, "new_value").value_or(0));
        d.tick = static_cast<std::uint64_t>(json_lite::get_number(sl, "tick").value_or(static_cast<double>(tick.sequence)));
        d.system_id = static_cast<std::uint32_t>(json_lite::get_number(sl, "system_id").value_or(0));
        tick.deltas.push(d);
        pos = found + 6;
    }
    return true;
}
} // namespace mf

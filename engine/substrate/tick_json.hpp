#pragma once
#include "engine/substrate/delta.hpp"
#include <sstream>
#include <string>
namespace mf {
inline std::string tick_to_json(const FieldTick& tick) {
    std::ostringstream os;
    os << "{\"sequence\":" << tick.sequence << ",\"time\":" << tick.time << ",\"dt\":" << tick.dt << ",\"deltas\":[";
    bool first = true;
    for (const auto& d : tick.deltas.items()) {
        if (!first) os << ',';
        first = false;
        os << "{\"cell\":{\"x\":" << d.cell.x << ",\"y\":" << d.cell.y << ",\"z\":" << d.cell.z << "}"
           << ",\"channel\":\"" << channel_name(d.channel) << "\""
           << ",\"old_value\":" << d.old_value << ",\"new_value\":" << d.new_value
           << ",\"system_id\":" << d.system_id << ",\"tick\":" << d.tick << "}";
    }
    os << "]}";
    return os.str();
}
} // namespace mf

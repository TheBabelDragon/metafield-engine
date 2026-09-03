#pragma once
#include "engine/core/types.hpp"
#include <string>
#include <cstdint>
#include <cmath>

namespace mf {

inline int parse_c3_id(const std::string& id) {
    if (id.rfind("c3-", 0) != 0) return -1;
    int n = 0;
    for (size_t i = 3; i < id.size(); ++i) {
        if (id[i] < '0' || id[i] > '9') break;
        n = n * 10 + (id[i] - '0');
    }
    return n;
}

inline Vec3 body_pose(const std::string& id) {
    const int c3 = parse_c3_id(id);
    if (c3 > 0) {
        const float ang = (float(c3 - 1) / 6.f) * 6.2831853f;
        return Vec3{std::cos(ang) * 2.15f, 0.f, std::sin(ang) * 2.15f};
    }
    std::uint32_t h = 2166136261u;
    for (unsigned char c : id) h = (h ^ c) * 16777619u;
    const float ang = float(h % 360) * 0.017453292f;
    const float r = (id.rfind("cyd", 0) == 0 || id.rfind("csi", 0) == 0) ? 3.05f : 1.7f;
    return Vec3{std::cos(ang) * r, 0.f, std::sin(ang) * r};
}

inline std::uint32_t body_color(const std::string& type, const std::string& id) {
    if (type == "c3_swarm" || id.rfind("c3-", 0) == 0) return 0x6ea8ffu;
    if (id.rfind("cyd", 0) == 0) return 0xffc46bu;
    return 0x2ee6a6u;
}

} // namespace mf

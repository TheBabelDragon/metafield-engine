#pragma once

#include "engine/world/world.hpp"
#include "engine/ecs/components.hpp"
#include "engine/fields/csi_field.hpp"
#include "engine/fields/field.hpp"

namespace mf {

inline CsiField& seed_demo_universe(World& world) {
    world.fields().create<OpticalField>(1);
    world.fields().create<ThermalField>(2);
    world.fields().create<AcousticField>(3);
    auto& csi = world.fields().create<CsiField>(10);

    auto spawn = [&](const char* name, const char* kind,
                     Vec3 pos, float sx, float sy, float sz, std::uint32_t color) {
        EntityID id = world.spawn();
        world.entities().add<Name>(id, Name{name});
        world.entities().add<Transform>(id, Transform{pos});
        world.entities().add<Renderable>(id, Renderable{kind, sx, sy, sz, color});
        return id;
    };

    spawn("house",  "house",  Vec3{-2.2f, 0.f, -1.6f}, 0.70f, 1.10f, 0.70f, 0x3a4656);
    spawn("tree",   "tree",   Vec3{ 2.4f, 0.f, -2.0f}, 0.18f, 1.60f, 0.18f, 0x2f4a38);
    spawn("canopy", "canopy", Vec3{ 2.4f, 1.4f,-2.0f}, 0.70f, 0.70f, 0.70f, 0x1f6b3e);
    spawn("light",  "light",  Vec3{-1.2f, 1.8f, -1.2f}, 0.12f, 0.12f, 0.12f, 0xffe08a);
    spawn("player", "player", Vec3{ 0.0f, 0.f,  2.2f}, 0.28f, 0.90f, 0.28f, 0x6aa0ff);
    spawn("npc",    "npc",    Vec3{ 1.4f, 0.f,  0.6f}, 0.26f, 0.85f, 0.26f, 0xd48ad0);
    return csi;
}

} // namespace mf

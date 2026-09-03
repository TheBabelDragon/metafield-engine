// hello-world — first smoke test of the MetaField Engine core
#include "engine/world/world.hpp"
#include "engine/core/types.hpp"
#include <iostream>
#include <cassert>
using namespace mf;
struct RigidBody {
    Vec3 velocity{0.f, 0.f, 0.f};
    float mass = 1.f;
};
struct Renderable { uint32_t mesh_id = 0; };
int main() {
    World world;
    world.time().refresh_scarcity();
    world.fields().create<OpticalField>(1);
    world.fields().create<ThermalField>(2);
    world.fields().create<AcousticField>(3);
    std::cout << "Fields registered: " << world.fields().count() << "\n";
    EntityID tree = world.spawn();
    world.entities().add<Transform>(tree, Transform{Vec3{5.f, 0.f, 3.f}});
    world.entities().add<Renderable>(tree, Renderable{});
    EntityID player = world.spawn();
    world.entities().add<Transform>(player, Transform{Vec3{0.f, 0.f, 0.f}});
    world.entities().add<RigidBody>(player, RigidBody{Vec3{1.f, 0.f, 0.f}, 80.f});
    EntityID sensor = world.spawn();
    world.entities().add<Transform>(sensor, Transform{Vec3{2.f, 1.f, 0.f}});
    std::cout << "Living entities: " << world.entities().living_count() << "\n";
    const Transform* t = world.entities().get<Transform>(player);
    assert(t);
    FieldSample optical  = world.sample(FieldType::Optical,  t->position);
    FieldSample thermal  = world.sample(FieldType::Thermal,  t->position);
    FieldSample acoustic = world.sample(FieldType::Acoustic, t->position);
    std::cout << "Optical intensity at player:  " << optical.intensity  << "\n";
    std::cout << "Thermal value at player:      " << thermal.scalar     << " \xc2\xb0C\n";
    std::cout << "Acoustic intensity at player: " << acoustic.intensity << "\n";
    for (int i = 0; i < 5; ++i) world.tick(1.0 / 60.0);
    std::cout << "Simulation time: " << world.time().simulation_time << " s\n";
    std::cout << "Tick count:      " << world.time().tick_count      << "\n";
    std::cout << "Clock:           " << world.time().scarcity.to_json() << "\n";
    std::cout << "Entities with Transform:\n";
    world.entities().each<Transform>([](EntityID id, Transform& tr) {
        std::cout << "  Entity " << id << " @ (" << tr.position.x << ", "
                  << tr.position.y << ", " << tr.position.z << ")\n";
    });
    std::cout << "\n[OK] MetaField Engine core smoke test passed.\n";
    return 0;
}

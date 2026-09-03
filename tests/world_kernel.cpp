#include "engine/world/world.hpp"
#include <cmath>
#include <cstdlib>
#include <iostream>
using namespace mf;
static int fails = 0;
static void check(bool ok, const char* msg) {
    if (!ok) { std::cerr << "FAIL " << msg << "\n"; ++fails; }
    else std::cout << "ok   " << msg << "\n";
}
int main() {
    unsetenv("METAFIELD_BTC_HEIGHT");
    unsetenv("METAFIELD_CLOCK_PATH");
    World world = World::create();
    WorldEvent ev;
    ev.type = EventType::Impulse;
    ev.cell = {3, 0, 3};
    ev.channel = Channel::Energy;
    ev.value = 0.5f;
    ev.name = "seed";
    world.push_event(ev);
    WorldTick tick = world.step(1.f / 60.f);
    check(tick.sequence == 1, "first step is WorldTick 1");
    check(tick.inputs.size() == 1, "tick records the event");
    check(tick.field_deltas.size() >= 1, "event becomes FieldDelta");
    check(world.substrate().sample({3, 0, 3}, Channel::Energy) > 0.f, "substrate committed");
    const auto hash = world.state_hash();
    check(hash != 0, "state_hash is nonzero after work");
    check(tick.state_hash == hash, "tick carries post-commit hash");
    World restored = replay(world.history());
    check(restored.state_hash() == hash, "replay(initial, history) matches live hash");
    check(restored.clock().tick == world.clock().tick, "replay clock tick matches");
    check(std::abs(restored.substrate().sample({3,0,3}, Channel::Energy) -
                   world.substrate().sample({3,0,3}, Channel::Energy)) < 1e-5f,
          "replay field energy matches");
    World again = World::create();
    again.push_event(ev);
    again.step(1.f / 60.f);
    check(again.state_hash() == hash, "same inputs + clock + rules → same hash");
    World idle = World::create();
    idle.step(1.f / 60.f);
    check(idle.state_hash() != hash, "different history → different hash");
    if (fails) { std::cerr << fails << " world-kernel test(s) failed\n"; return 1; }
    std::cout << "[OK] world kernel\n";
    return 0;
}

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
    unsetenv("METAFIELD_ALLOW_SYNTHETIC");

    World gated = World::create();
    WorldEvent blocked;
    blocked.value = 1.f;
    check(!gated.push_event(blocked), "unlabeled event is synthetic and rejected");
    check(gated.rejected_events() == 1, "rejection is counted");

    Observation fake;
    fake.quantity = Quantity::Temperature;
    fake.value = 293.15;
    fake.unit = quantity_unit(fake.quantity);
    fake.provenance = Provenance::synthetic("model-b");
    fake.where = CellCoord{1, 0, 1};
    check(!gated.admit(fake), "synthetic observation rejected by default");

    Observation live;
    live.quantity = Quantity::Temperature;
    live.value = 0.25;
    live.unit = "K";
    live.uncertainty = 0.01;
    live.provenance = Provenance::physical("thermocouple-a", "max31855");
    live.where = CellCoord{3, 0, 3};
    check(gated.admit(live), "physical observation admitted");

    WorldTick tick = gated.step(1.f / 60.f);
    check(tick.sequence == 1, "first step is WorldTick 1");
    check(tick.inputs.size() == 1, "tick records the physical event");
    check(tick.field_deltas.size() >= 1, "event becomes FieldDelta");
    check(gated.substrate().sample({3, 0, 3}, Channel::Temperature) > 0.f, "substrate committed");

    const auto hash = gated.state_hash();
    check(hash != 0, "state_hash is nonzero after work");
    check(tick.state_hash == hash, "tick carries post-commit hash");

    World restored = replay(gated.history());
    check(restored.state_hash() == hash, "replay matches live hash");
    check(restored.clock().tick == gated.clock().tick, "replay clock tick matches");

    World again = World::create();
    again.admit(live);
    again.step(1.f / 60.f);
    check(again.state_hash() == hash, "same physical inputs → same hash");

    World idle = World::create();
    idle.step(1.f / 60.f);
    check(idle.state_hash() != hash, "different history → different hash");

    World model = World::create();
    model.allow_synthetic(true);
    check(model.admit(fake), "synthetic admitted only when enabled");

    if (fails) { std::cerr << fails << " world-kernel test(s) failed\n"; return 1; }
    std::cout << "[OK] world kernel\n";
    return 0;
}

#include "engine/substrate/scheduler.hpp"
#include "engine/substrate/replay.hpp"
#include "engine/substrate/tick_json.hpp"
#include <iostream>
#include <iomanip>
using namespace mf;
int main() {
    VoxelField field;
    field.reserve_box({0,0,0},{7,0,7});
    field.write({3,0,3}, Channel::Temperature, 1.0f);
    field.write({4,0,3}, Channel::Temperature, 0.8f);
    field.write({3,0,3}, Channel::Information, 0.74f);
    for (int z=0;z<=7;++z) for (int x=0;x<=7;++x)
        field.write({x,0,z}, Channel::MomentumX, 0.35f);
    const auto snap0 = capture(field);
    FieldTickStream stream;
    FieldScheduler sched;
    sched.add(std::make_unique<DiffusionSystem>(0.5f, Boundary::Closed));
    sched.add(std::make_unique<InformationDecaySystem>(0.15f));
    sched.add(std::make_unique<AdvectionSystem>());
    sched.attach(stream);
    std::cout << "MetaField substrate\n systems=" << sched.system_count() << "\n";
    for (int i=0;i<8;++i) {
        auto tick = sched.step(field, 0.16f);
        std::size_t adv=0; for (const auto& d: tick.deltas.items()) if (d.system_id==SYS_ADVECTION) ++adv;
        std::cout << " TICK " << tick.sequence
                  << "  tempΔ=" << tick.deltas.count_channel(Channel::Temperature)
                  << "  infoΔ=" << tick.deltas.count_channel(Channel::Information)
                  << "  advΔ=" << adv
                  << "  heat=" << std::fixed << std::setprecision(3) << field.sum(Channel::Temperature)
                  << "  info=" << field.sum(Channel::Information) << "\n";
    }
    std::cout << " replay match=" << (fields_equivalent(field, replay(snap0, stream.ticks())) ? "yes" : "NO") << "\n";
    std::cout << "[OK] advection on FieldTick spine\n";
    return 0;
}

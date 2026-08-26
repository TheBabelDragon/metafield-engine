#include "engine/substrate/scheduler.hpp"
#include <iostream>
#include <iomanip>
using namespace mf;
int main() {
    VoxelField field;
    field.reserve_box({0,0,0},{7,0,7});
    field.write({3,0,3}, Channel::Temperature, 1.0f);
    field.write({4,0,3}, Channel::Temperature, 0.8f);
    field.write({3,0,3}, Channel::Information, 0.74f);
    FieldScheduler sched;
    sched.add(std::make_unique<DiffusionSystem>(0.8f, Boundary::Closed));
    sched.add(std::make_unique<InformationDecaySystem>(0.15f));
    std::cout << "MetaField substrate\n systems=" << sched.system_count() << "\n";
    for (int i=0;i<8;++i) {
        auto tick = sched.step(field, 0.16f);
        std::cout << " TICK " << tick.sequence
                  << "  tempΔ=" << tick.deltas.count_channel(Channel::Temperature)
                  << "  infoΔ=" << tick.deltas.count_channel(Channel::Information)
                  << "  heat=" << std::fixed << std::setprecision(3) << field.sum(Channel::Temperature)
                  << "  info=" << field.sum(Channel::Information) << "\n";
    }
    if (const auto* d = sched.last().deltas.find({3,0,3}, Channel::Temperature))
        std::cout << " CELL (3,0,3) temperature " << d->old_value << " -> " << d->new_value
                  << "  system=" << system_name(d->system_id) << "  tick=" << d->tick << "\n";
    if (const auto* d = sched.last().deltas.find({3,0,3}, Channel::Information))
        std::cout << " CELL (3,0,3) information " << d->old_value << " -> " << d->new_value
                  << "  system=" << system_name(d->system_id) << "  tick=" << d->tick << "\n";
    std::cout << "[OK] FieldTick commit\n";
    return 0;
}

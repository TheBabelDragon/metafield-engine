#include "engine/substrate/scheduler.hpp"
#include <iostream>
#include <iomanip>
using namespace mf;
int main() {
    VoxelField field;
    field.reserve_box({0,0,0},{7,0,7});
    field.write({3,0,3}, Channel::Temperature, 1.0f);
    field.write({4,0,3}, Channel::Temperature, 0.8f);
    FieldScheduler sched;
    sched.add(std::make_unique<DiffusionSystem>(0.8f));
    std::cout << "MetaField substrate\n chunks=" << field.chunk_count()
              << " systems=" << sched.system_count() << "\n";
    for (int tick = 0; tick < 8; ++tick) {
        auto deltas = sched.tick(field, 0.16f);
        float energy = 0.f, peak = 0.f;
        field.each_existing([&](CellCoord, const CellState& cell) {
            const float t = cell.get(Channel::Temperature);
            energy += t; if (t > peak) peak = t;
        });
        std::cout << " tick " << tick << "  deltas=" << deltas.size()
                  << "  heat=" << std::fixed << std::setprecision(3) << energy
                  << "  peak=" << peak << "\n";
    }
    std::cout << "\n temperature slice y=0:\n";
    for (int z = 0; z < 8; ++z) {
        std::cout << "  ";
        for (int x = 0; x < 8; ++x) {
            const float t = field.sample({x,0,z}, Channel::Temperature);
            std::cout << (t<0.02f?'.':t<0.15f?':':t<0.35f?'+':t<0.6f?'*':'#');
        }
        std::cout << "\n";
    }
    std::cout << "[OK] FieldDelta spine + diffusion\n";
    return 0;
}

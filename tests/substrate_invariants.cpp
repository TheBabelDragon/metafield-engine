#include "engine/substrate/scheduler.hpp"
#include <cmath>
#include <iostream>
using namespace mf;
static int fails = 0;
static void check(bool ok, const char* msg) {
    if (!ok) { std::cerr << "FAIL " << msg << "\n"; ++fails; }
    else std::cout << "ok   " << msg << "\n";
}
static VoxelField seeded() {
    VoxelField f; f.reserve_box({0,0,0},{7,0,7});
    f.write({3,0,3}, Channel::Temperature, 1.0f);
    f.write({4,0,3}, Channel::Temperature, 0.8f);
    f.write({3,0,3}, Channel::Information, 0.74f);
    f.write({4,0,3}, Channel::Information, 0.50f);
    return f;
}
static FieldScheduler make_sched() {
    FieldScheduler s;
    s.add(std::make_unique<DiffusionSystem>(0.8f, Boundary::Closed));
    s.add(std::make_unique<InformationDecaySystem>(0.15f));
    return s;
}
int main() {
    { auto f=seeded(); float before=f.sum(Channel::Temperature); auto s=make_sched(); s.step(f,0.16f);
      check(std::fabs(before-f.sum(Channel::Temperature))<1e-4f, "diffusion closed boundary conserves heat"); }
    { auto f=seeded(); auto s=make_sched(); float before=f.sum(Channel::Information); auto tick=s.step(f,0.16f);
      check(f.sum(Channel::Information)<=before+1e-6f, "information does not spontaneously increase");
      bool all=true; for (const auto& d:tick.deltas.items()) if (d.channel==Channel::Information && d.new_value>d.old_value+1e-8f) all=false;
      check(all, "information deltas are non-increasing"); }
    { auto f=seeded(); auto s=make_sched(); auto tick=s.step(f,0.16f);
      check(tick.sequence==1, "first committed tick is sequence 1");
      check(tick.deltas.count_channel(Channel::Temperature)>0 && tick.deltas.count_channel(Channel::Information)>0,
            "one tick contains both temperature and information deltas");
      bool ids=true; for (const auto& d:tick.deltas.items()) {
        if (d.channel==Channel::Temperature && d.system_id!=SYS_DIFFUSION) ids=false;
        if (d.channel==Channel::Information && d.system_id!=SYS_INFO_DECAY) ids=false;
        if (d.tick!=tick.sequence) ids=false; }
      check(ids, "deltas carry system_id and tick"); }
    { auto f=seeded(); auto s=make_sched(); auto tick=s.step(f,0.16f); auto f2=seeded(); bool ok=true;
      for (const auto& d:tick.deltas.items()) if (std::fabs(f2.sample(d.cell,d.channel)-d.old_value)>1e-5f) ok=false;
      check(ok, "old_value matches field before application"); }
    { auto fa=seeded(), fb=seeded(); auto sa=make_sched(), sb=make_sched();
      auto ta=sa.step(fa,0.16f), tb=sb.step(fb,0.16f);
      check(ta.sequence==tb.sequence && ta.deltas.size()==tb.deltas.size(), "identical inputs produce same tick size");
      bool same=true;
      for (size_t i=0;i<ta.deltas.size();++i) {
        const auto& a=ta.deltas.items()[i]; const auto& b=tb.deltas.items()[i];
        if (!(a.cell==b.cell)||a.channel!=b.channel||std::fabs(a.old_value-b.old_value)>1e-6f||std::fabs(a.new_value-b.new_value)>1e-6f||a.system_id!=b.system_id) same=false;
      }
      check(same, "identical inputs produce identical ordered FieldDeltaList");
      bool fsame=true; fa.each_cell_sorted([&](CellCoord c, const CellState& ca){
        if (std::fabs(ca.get(Channel::Temperature)-fb.sample(c,Channel::Temperature))>1e-6f) fsame=false;
        if (std::fabs(ca.get(Channel::Information)-fb.sample(c,Channel::Information))>1e-6f) fsame=false; });
      check(fsame, "identical inputs produce identical final Field"); }
    { auto f=seeded(); float t0=f.sum(Channel::Temperature), i0=f.sum(Channel::Information);
      FieldView view(f); FieldDeltaList out; DiffusionSystem diff(0.8f, Boundary::Closed); InformationDecaySystem decay(0.15f);
      diff.evaluate(view,out,0.16f); decay.evaluate(view,out,0.16f);
      check(std::fabs(f.sum(Channel::Temperature)-t0)<1e-8f && std::fabs(f.sum(Channel::Information)-i0)<1e-8f, "evaluate does not mutate Field"); }
    if (fails) { std::cerr << fails << " invariant(s) failed\n"; return 1; }
    std::cout << "[OK] substrate invariants\n"; return 0;
}

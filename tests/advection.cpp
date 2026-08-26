#include "engine/substrate/scheduler.hpp"
#include "engine/substrate/replay.hpp"
#include "engine/substrate/query.hpp"
#include <cmath>
#include <iostream>
using namespace mf;
static int fails=0;
static void check(bool ok,const char* msg){ if(!ok){std::cerr<<"FAIL "<<msg<<"\n";++fails;} else std::cout<<"ok   "<<msg<<"\n"; }
static VoxelField blob(){
    VoxelField f; f.reserve_box({0,0,0},{7,0,7});
    f.write({2,0,3}, Channel::Temperature, 1.0f);
    f.write({3,0,3}, Channel::Temperature, 0.6f);
    for(int z=0;z<=7;++z) for(int x=0;x<=7;++x) f.write({x,0,z}, Channel::MomentumX, 0.35f);
    return f;
}
int main(){
    { auto f=blob(); const float heat0=f.sum(Channel::Temperature);
      FieldScheduler s; s.add(std::make_unique<AdvectionSystem>());
      for(int i=0;i<10;++i) s.step(f,0.2f);
      check(std::fabs(f.sum(Channel::Temperature)-heat0)<1e-4f, "closed donor-cell advection conserves temperature");
      check(f.sample({2,0,3}, Channel::Temperature) < 1.0f-1e-4f, "hotspot moves downwind"); }
    { auto live=blob(); live.write({3,0,3}, Channel::Information, 0.7f);
      const auto snap0=capture(live); FieldTickStream stream; FieldScheduler s;
      s.add(std::make_unique<DiffusionSystem>(0.4f, Boundary::Closed));
      s.add(std::make_unique<InformationDecaySystem>(0.1f));
      s.add(std::make_unique<AdvectionSystem>()); s.attach(stream);
      FieldView view(live); FieldDeltaList out; AdvectionSystem adv; const float t0=live.sum(Channel::Temperature);
      adv.evaluate(view,out,0.2f);
      check(std::fabs(live.sum(Channel::Temperature)-t0)<1e-8f, "advection evaluate does not mutate Field");
      for(int i=0;i<12;++i) s.step(live,0.16f);
      check(fields_equivalent(live, replay(snap0, stream.ticks())), "replay after diffusion+decay+advection");
      check(!deltas_for_system(stream, SYS_ADVECTION).empty(), "stream contains advection deltas"); }
    if(fails){ std::cerr<<fails<<" failed\n"; return 1; }
    std::cout<<"[OK] advection\n"; return 0;
}

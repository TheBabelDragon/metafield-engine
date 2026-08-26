#include "engine/substrate/scheduler.hpp"
#include "engine/substrate/replay.hpp"
#include "engine/substrate/query.hpp"
#include "engine/substrate/tick_json.hpp"
#include <iostream>
using namespace mf;
static int fails=0;
static void check(bool ok,const char* msg){ if(!ok){std::cerr<<"FAIL "<<msg<<"\n";++fails;} else std::cout<<"ok   "<<msg<<"\n"; }
static VoxelField seeded(){
    VoxelField f; f.reserve_box({0,0,0},{7,0,7});
    f.write({3,0,3},Channel::Temperature,1.0f);
    f.write({4,0,3},Channel::Temperature,0.8f);
    f.write({3,0,3},Channel::Information,0.74f);
    f.write({4,0,3},Channel::Information,0.50f);
    return f;
}
int main(){
    auto live=seeded(); const auto snap0=capture(live);
    FieldTickStream stream; DeltaCounter counter; stream.subscribe(counter);
    FieldScheduler sched;
    sched.add(std::make_unique<DiffusionSystem>(0.8f, Boundary::Closed));
    sched.add(std::make_unique<InformationDecaySystem>(0.15f));
    sched.attach(stream);
    for(int i=0;i<12;++i) sched.step(live,0.16f);
    check(stream.size()==12,"stream has N committed ticks");
    check(counter.ticks==12,"subscriber saw every committed tick");
    check(counter.temperature>0&&counter.information>0,"subscriber saw both channels");
    std::size_t lt=0,li=0; for(const auto& t:stream.ticks()){ lt+=t.deltas.count_channel(Channel::Temperature); li+=t.deltas.count_channel(Channel::Information); }
    check(counter.temperature==lt&&counter.information==li,"subscriber counts match committed ticks");
    check(fields_equivalent(live, replay(snap0, stream.ticks())), "snapshot + ticks reconstruct live Field @ N");
    check(!deltas_for_cell(stream,{3,0,3}).empty(),"query: ticks affecting (3,0,3)");
    auto after=deltas_for_channel_after(stream,Channel::Temperature,3);
    bool aok=true; for(const auto& d:after) if(d.tick<=3||d.channel!=Channel::Temperature) aok=false;
    check(aok&&!after.empty(),"query: temperature after tick 3");
    auto diff=deltas_for_system(stream,SYS_DIFFUSION);
    bool dok=true; for(const auto& d:diff) if(d.system_id!=SYS_DIFFUSION) dok=false;
    check(dok&&!diff.empty(),"query: all diffusion changes");
    auto info=deltas_for_system(stream,SYS_INFO_DECAY);
    bool iok=true; for(const auto& d:info) if(d.channel!=Channel::Information) iok=false;
    check(iok&&!info.empty(),"query: all information changes");
    check(!deltas_in_region(stream,{3,0,3},{4,0,3}).empty(),"query: region");
    const auto js=tick_to_json(stream.ticks().back());
    check(js.find("\"sequence\":" )!=std::string::npos && js.find("\"deltas\":" )!=std::string::npos,"canonical tick JSON");
    if(fails){ std::cerr<<fails<<" failed\n"; return 1; }
    std::cout<<"[OK] FieldTick replay + subscriptions\n"; return 0;
}

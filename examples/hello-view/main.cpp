// hello-view — live ESP32 CSI + FieldTick + arrow-key player
#include "engine/world/world.hpp"
#include "engine/world/demo_universe.hpp"
#include "engine/ecs/components.hpp"
#include "engine/fields/csi_field.hpp"
#include "engine/fields/csi_infer.hpp"
#include "engine/ingest/csi_parse.hpp"
#include "engine/ingest/jsonl_tail.hpp"
#include "engine/renderer/hud_server.hpp"
#include "engine/substrate/scheduler.hpp"
#include "engine/substrate/tick_json.hpp"
#include <atomic>
#include <chrono>
#include <cmath>
#include <csignal>
#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <iomanip>
using namespace mf;
static volatile std::sig_atomic_t g_run=1; static void on_sig(int){g_run=0;}
static const char* default_jsonl(){ const char* e=std::getenv("METAFIELD_CSI_JSONL"); return (e&&e[0])?e:"/tmp/metafield/csi.jsonl"; }
static std::string json_escape(const std::string& s){ std::string o; for(char c:s){ if(c=='"'||c=='\\'){o.push_back('\\');o.push_back(c);} else o.push_back(c);} return o; }
static std::string farr(const std::vector<float>& v){ std::ostringstream os; os<<'['; for(size_t i=0;i<v.size();++i){ if(i) os<<','; os<<v[i]; } os<<']'; return os.str(); }
static std::string hex_color(std::uint32_t c){ std::ostringstream os; os<<'#'<<std::hex<<std::setw(6)<<std::setfill('0')<<(c&0xffffff); return os.str(); }
static Vec3 sensor_pos(const std::string& id){ std::uint32_t h=2166136261u; for(unsigned char c:id) h=(h^c)*16777619u; float ang=float(h%360)*0.017453292f, r=1.7f+float(h%90)/90.f; return Vec3{std::cos(ang)*r,0.f,std::sin(ang)*r}; }
static EntityID upsert_sensor(World& world,std::unordered_map<std::string,EntityID>& sensors,const FieldObservation& obs){
    auto it=sensors.find(obs.body_id); if(it!=sensors.end()) return it->second;
    EntityID id=world.spawn();
    world.entities().add<Name>(id,Name{obs.body_id});
    world.entities().add<Transform>(id,Transform{sensor_pos(obs.body_id)});
    world.entities().add<Renderable>(id,Renderable{"sensor",0.32f,0.6f,0.32f,0x2ee6a6u});
    world.entities().add<SensorTag>(id,SensorTag{obs.body_id});
    sensors[obs.body_id]=id; return id;
}
static float clampf(float v,float lo,float hi){ return v<lo?lo:(v>hi?hi:v); }
int main(int argc,char** argv){
    std::signal(SIGINT,on_sig); std::signal(SIGTERM,on_sig);
    std::string path=default_jsonl(); int seconds=0, port=8765;
    for(int i=1;i<argc;++i){ std::string a=argv[i];
        if(a=="--synth"){ std::cerr<<"--synth removed. Flash a real ESP32 CSI node.\n"; return 2; }
        else if(a=="--file"&&i+1<argc) path=argv[++i];
        else if(a=="--seconds"&&i+1<argc) seconds=std::atoi(argv[++i]);
        else if(a=="--port"&&i+1<argc) port=std::atoi(argv[++i]);
    }
    World world; auto& csi_field=seed_demo_universe(world);
    JsonlTail tail(path,true); bool file_live=tail.file_exists();
    std::unordered_map<std::string,EntityID> sensors; CsiInferencer infer; std::mutex infer_mu; CsiEstimate last_est; std::atomic<bool> est_mode{false};
    std::atomic<bool> player_manual{false};
    std::atomic<float> player_x{0.f}, player_z{2.2f};
    VoxelField vox; vox.reserve_box({0,0,0},{7,0,7});
    vox.write({3,0,3}, Channel::Temperature, 42.81f);
    vox.write({3,0,3}, Channel::Information, 0.74f);
    vox.write({4,0,3}, Channel::Temperature, 20.0f);
    FieldScheduler fsched;
    fsched.add(std::make_unique<DiffusionSystem>(0.45f, Boundary::Closed));
    fsched.add(std::make_unique<InformationDecaySystem>(0.12f));
    FieldTick last_tick; std::mutex tick_mu;
    HudServer hud; const bool hud_ok=hud.start(port,[&](){
        auto latest=csi_field.latest(); auto bodies=csi_field.bodies(); CsiEstimate est; {std::lock_guard<std::mutex> g(infer_mu); est=last_est;}
        std::ostringstream os;
        os<<"{\"packets\":"<<csi_field.packet_count()<<",\"sim_t\":"<<world.time().simulation_time
          <<",\"view\":\""<<(est_mode.load()?"est":"live")<<"\""
          <<",\"live\":"<<(file_live?"true":"false")
          <<",\"jsonl_missing\":"<<(tail.file_exists()?"false":"true")
          <<",\"player_manual\":"<<(player_manual.load()?"true":"false")
          <<",\"entities\":"<<world.entities().living_count()
          <<",\"latest\":{\"body_id\":\""<<json_escape(latest.body_id)<<"\",\"synthetic\":false"
          <<",\"rssi\":"<<latest.region("rssi")<<",\"energy\":"<<latest.region("csi_energy")
          <<",\"spread\":"<<latest.region("csi_spread")<<",\"csi\":"<<farr(latest.csi)
          <<"},\"estimate\":{\"live\":"<<(est.live?"true":"false")<<",\"motion\":"<<est.motion
          <<",\"energy\":"<<est.energy<<",\"gw\":"<<est.gw<<",\"gz\":"<<est.gz
          <<",\"grid\":"<<farr(est.grid)<<",\"blobs\":[";
        for(size_t i=0;i<est.blobs.size();++i){ const auto& b=est.blobs[i]; if(i) os<<',';
            os<<"{\"id\":"<<b.id<<",\"x\":"<<b.x<<",\"z\":"<<b.z<<",\"vx\":"<<b.vx<<",\"vz\":"<<b.vz
              <<",\"rx\":"<<b.rx<<",\"rz\":"<<b.rz<<",\"angle\":"<<b.angle<<",\"energy\":"<<b.energy
              <<",\"motion\":"<<b.motion<<",\"age\":"<<b.age<<",\"trail\":"<<farr(b.trail)<<",\"contour\":"<<farr(b.contour)<<"}"; }
        FieldTick tk; {std::lock_guard<std::mutex> g(tick_mu); tk=last_tick;}
        os<<"]},\"tick\":"<<tick_to_json(tk);
        os<<",\"world\":["; bool first=true;
        world.entities().each<Name,Transform,Renderable>([&](EntityID id,Name& name,Transform& tr,Renderable& rend){
            if(!first) { os<<','; } first=false; float energy=0,rssi=0;
            if(rend.kind=="sensor"){ auto bit=bodies.find(name.value); if(bit!=bodies.end()){ energy=bit->second.last.region("csi_energy"); rssi=bit->second.last.region("rssi"); } }
            os<<"{\"id\":"<<id<<",\"name\":\""<<json_escape(name.value)<<"\",\"kind\":\""<<json_escape(rend.kind)<<"\""
              <<",\"x\":"<<tr.position.x<<",\"y\":"<<tr.position.y<<",\"z\":"<<tr.position.z
              <<",\"sx\":"<<rend.sx<<",\"sy\":"<<rend.sy<<",\"sz\":"<<rend.sz
              <<",\"energy\":"<<energy<<",\"rssi\":"<<rssi<<",\"synthetic\":false"
              <<",\"color\":\""<<hex_color(rend.color)<<"\"}";
        }); os<<"]}"; return os.str();
    },[&](std::string_view path){
        if(path.find("view=est")!=std::string_view::npos) est_mode.store(true);
        auto apply=[&](float dx,float dz){
            player_manual.store(true);
            player_x.store(clampf(player_x.load()+dx,-3.2f,3.2f));
            player_z.store(clampf(player_z.load()+dz,-3.2f,3.2f));
        };
        if(path.find("move=up")!=std::string_view::npos) apply(0.f,-0.28f);
        else if(path.find("move=down")!=std::string_view::npos) apply(0.f,0.28f);
        else if(path.find("move=left")!=std::string_view::npos) apply(-0.28f,0.f);
        else if(path.find("move=right")!=std::string_view::npos) apply(0.28f,0.f);
        std::ostringstream os;
        os<<"{\"view\":\""<<(est_mode.load()?"est":"live")<<"\",\"player_manual\":"
          <<(player_manual.load()?"true":"false")<<",\"x\":"<<player_x.load()<<",\"z\":"<<player_z.load()<<"}";
        return os.str();
    });
    const std::string url="http://127.0.0.1:"+std::to_string(port);
    std::cout<<"\n================================================\n MetaField Engine HUD\n "<<url<<(hud_ok?"\n":"  [bind failed]\n")<<"================================================\n";
    std::cout<<" jsonl : "<<path<<(tail.file_exists()?"  [present]\n":"  [missing]\n")
             <<" mode  : LIVE ESP32 only (synthetic_cyd removed)\n"
             <<" keys  : arrows / WASD move player\n"
             <<" stop  : Ctrl+C\n\n";
    using clock=std::chrono::steady_clock; const auto start=clock::now();
    auto next_status=start;
    std::uint64_t last_pk=0; bool announced_live=false, announced_player=false;
    while(g_run){
        if(seconds>0 && std::chrono::duration_cast<std::chrono::seconds>(clock::now()-start).count()>=seconds) break;
        if(!file_live && tail.file_exists()) file_live=true;
        if(file_live){
            for(int n=0;n<64;++n){
                auto line=tail.poll(); if(!line) break;
                auto obs=parse_csi_line(*line); if(!obs.valid) continue;
                if(obs.synthetic || obs.source_class==SourceClass::Synthetic) continue;
                if(obs.body_id.rfind("synthetic",0)==0) continue;
                csi_field.ingest(obs); upsert_sensor(world,sensors,obs); infer.push(obs);
            }
        }
        { std::lock_guard<std::mutex> g(infer_mu); last_est=infer.estimate(); last_est.live = file_live; }
        const auto latest=csi_field.latest();
        const auto pk=csi_field.packet_count();
        if(pk>0 && !latest.synthetic && !announced_live){
            announced_live=true;
            std::cout<<"[LIVE] PASS  real CSI  body="<<latest.body_id<<"  packets="<<pk<<"\n"<<std::flush;
        }
        if(player_manual.load() && !announced_player){
            announced_player=true;
            std::cout<<"[player] keys active\n"<<std::flush;
        }
        const auto now=clock::now();
        if(now>=next_status){
            next_status=now+std::chrono::seconds(2);
            const bool live=pk>0 && !latest.synthetic;
            std::cout<<"[status] "<<(live?"LIVE":"WAIT")
                     <<"  packets="<<pk
                     <<"  body="<<(latest.body_id.empty()?"-":latest.body_id)
                     <<(pk>last_pk?"  +":"")
                     <<"\n"<<std::flush;
            last_pk=pk;
        }
        const float t=float(world.time().simulation_time); const auto bodies=csi_field.bodies();
        world.entities().each<Name,Transform,Renderable>([&](EntityID,Name& name,Transform& tr,Renderable& rend){
            if(name.value=="player"){
                if(player_manual.load()){ tr.position.x=player_x.load(); tr.position.z=player_z.load(); }
                else { tr.position.x=std::sin(t*0.35f)*1.6f; tr.position.z=2.2f+std::cos(t*0.35f)*0.4f; player_x.store(tr.position.x); player_z.store(tr.position.z); }
            } else if(name.value=="npc") tr.position.x=1.4f+std::sin(t*0.55f)*0.6f;
            else if(rend.kind=="sensor"){
                auto bit=bodies.find(name.value);
                float e=bit==bodies.end()?0.f:bit->second.last.region("csi_energy");
                rend.sy=0.35f+e*1.8f; rend.color=0x2ee6a6u;
            }
        });
        { auto tk=fsched.step(vox, 1.f/60.f); std::lock_guard<std::mutex> g(tick_mu); last_tick=std::move(tk);} world.tick(1.0/60.0); std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    hud.stop();
    const auto latest=csi_field.latest();
    const bool pass=csi_field.packet_count()>0 && !latest.synthetic;
    std::cout<<(pass?"[LIVE] PASS":"[LIVE] WAIT")<<"  packets="<<csi_field.packet_count()<<"\n"<<url<<"\n";
    return hud_ok?0:1;
}

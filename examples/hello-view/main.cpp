// hello-view — World-owned scene + CSI ingest + localhost 3D HUD
#include "engine/world/world.hpp"
#include "engine/world/demo_universe.hpp"
#include "engine/ecs/components.hpp"
#include "engine/fields/csi_field.hpp"
#include "engine/ingest/csi_parse.hpp"
#include "engine/ingest/jsonl_tail.hpp"
#include "engine/ingest/synthetic_csi.hpp"
#include "engine/renderer/hud_server.hpp"

#include <chrono>
#include <cmath>
#include <csignal>
#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <iomanip>

using namespace mf;
static volatile std::sig_atomic_t g_run = 1;
static void on_sig(int) { g_run = 0; }

static const char* default_jsonl() {
    const char* env = std::getenv("METAFIELD_CSI_JSONL");
    if (env && env[0]) return env;
    return "/tmp/metafield/csi.jsonl";
}
static std::string json_escape(const std::string& s) {
    std::string o; o.reserve(s.size());
    for (char c : s) {
        if (c=='"' || c=='\\') { o.push_back('\\'); o.push_back(c); }
        else o.push_back(c);
    }
    return o;
}
static std::string csi_array(const std::vector<float>& v) {
    std::ostringstream os; os << '[';
    for (size_t i=0;i<v.size();++i){ if(i) os<<','; os<<v[i]; }
    os << ']'; return os.str();
}
static std::string hex_color(std::uint32_t c) {
    std::ostringstream os;
    os << '#' << std::hex << std::setw(6) << std::setfill('0') << (c & 0xffffff);
    return os.str();
}
static Vec3 sensor_pos(const std::string& id) {
    std::uint32_t h = 2166136261u;
    for (unsigned char c : id) h = (h ^ c) * 16777619u;
    const float ang = static_cast<float>(h % 360) * 0.017453292f;
    const float r = 1.7f + static_cast<float>(h % 90) / 90.f;
    return Vec3{std::cos(ang) * r, 0.f, std::sin(ang) * r};
}
static EntityID upsert_sensor(World& world,
                              std::unordered_map<std::string, EntityID>& sensors,
                              const FieldObservation& obs) {
    auto it = sensors.find(obs.body_id);
    if (it != sensors.end()) return it->second;
    EntityID id = world.spawn();
    world.entities().add<Name>(id, Name{obs.body_id});
    world.entities().add<Transform>(id, Transform{sensor_pos(obs.body_id)});
    world.entities().add<Renderable>(id, Renderable{
        "sensor", 0.32f, 0.6f, 0.32f,
        obs.synthetic ? 0xc9842au : 0x2ee6a6u
    });
    world.entities().add<SensorTag>(id, SensorTag{obs.body_id});
    sensors[obs.body_id] = id;
    return id;
}

int main(int argc, char** argv) {
    std::signal(SIGINT, on_sig); std::signal(SIGTERM, on_sig);
    std::string path = default_jsonl(); bool force_synth=false; int seconds=0; int port=8765;
    for (int i=1;i<argc;++i){
        std::string a=argv[i];
        if(a=="--synth") force_synth=true;
        else if(a=="--file"&&i+1<argc) path=argv[++i];
        else if(a=="--seconds"&&i+1<argc) seconds=std::atoi(argv[++i]);
        else if(a=="--port"&&i+1<argc) port=std::atoi(argv[++i]);
        else if(a=="--help"||a=="-h"){ std::cout<<"hello_view\n"; return 0; }
    }

    World world;
    auto& csi_field = seed_demo_universe(world);
    JsonlTail tail(path, true);
    bool live = !force_synth && tail.file_exists();
    std::unordered_map<std::string, EntityID> sensors;

    HudServer hud;
    const bool hud_ok = hud.start(port, [&]() {
        auto latest = csi_field.latest();
        auto bodies = csi_field.bodies();
        std::ostringstream os;
        os << "{\"packets\":" << csi_field.packet_count()
           << ",\"sim_t\":" << world.time().simulation_time
           << ",\"live\":" << (live?"true":"false")
           << ",\"jsonl_missing\":" << (tail.file_exists()?"false":"true")
           << ",\"entities\":" << world.entities().living_count()
           << ",\"latest\":{"
           << "\"body_id\":\"" << json_escape(latest.body_id) << "\","
           << "\"synthetic\":" << (latest.synthetic?"true":"false") << ","
           << "\"rssi\":" << latest.region("rssi") << ","
           << "\"energy\":" << latest.region("csi_energy") << ","
           << "\"spread\":" << latest.region("csi_spread") << ","
           << "\"csi\":" << csi_array(latest.csi)
           << "},\"world\":[";
        bool first = true;
        world.entities().each<Name, Transform, Renderable>([&](EntityID id, Name& name, Transform& tr, Renderable& rend){
            if (!first) {
                os << ',';
            }
            first = false;
            float energy = 0.f, rssi = 0.f;
            bool syn = false;
            if (rend.kind == "sensor") {
                auto bit = bodies.find(name.value);
                if (bit != bodies.end()) {
                    energy = bit->second.last.region("csi_energy");
                    rssi = bit->second.last.region("rssi");
                    syn = bit->second.last.synthetic;
                }
            }
            os << "{\"id\":" << id
               << ",\"name\":\"" << json_escape(name.value) << "\""
               << ",\"kind\":\"" << json_escape(rend.kind) << "\""
               << ",\"x\":" << tr.position.x
               << ",\"y\":" << tr.position.y
               << ",\"z\":" << tr.position.z
               << ",\"sx\":" << rend.sx
               << ",\"sy\":" << rend.sy
               << ",\"sz\":" << rend.sz
               << ",\"energy\":" << energy
               << ",\"rssi\":" << rssi
               << ",\"synthetic\":" << (syn?"true":"false")
               << ",\"color\":\"" << hex_color(rend.color) << "\"}";
        });
        os << "]}";
        return os.str();
    });

    const std::string url = "http://127.0.0.1:" + std::to_string(port);
    std::cout << "\n================================================\n";
    std::cout << " MetaField Engine HUD\n " << url << (hud_ok?"\n":"  [bind failed]\n");
    std::cout << "================================================\n";
    std::cout << " jsonl : " << path << (tail.file_exists()?"  [present]\n":"  [missing]\n");
    if (!tail.file_exists()) {
        std::cout << " hint  : JSONL missing is normal until the CSI snake writes it.\n";
        std::cout << "           python -m observer.metafield_bridge --udp --out /tmp/metafield/csi.jsonl\n";
    }
    std::cout << " mode  : " << (live?"LIVE follow":"synthetic") << "\n";
    std::cout << " world : " << world.entities().living_count() << " entities\n";
    std::cout << " cam   : drag orbit  wheel zoom  double-click reset\n";
    std::cout << " stop  : Ctrl+C\n\n";
    std::cout.flush();

    using clock = std::chrono::steady_clock;
    const auto start = clock::now();
    auto next_synth = start;
    std::uint64_t synth_tick = 0;

    while (g_run) {
        if (seconds > 0) {
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(clock::now()-start).count();
            if (elapsed >= seconds) break;
        }
        if (!force_synth && !live && tail.file_exists()) {
            live = true;
            std::cout << "[ingest] live JSONL appeared — " << url << "\n";
        }
        if (live) {
            for (int n=0;n<64;++n) {
                auto line = tail.poll();
                if (!line) break;
                auto obs = parse_csi_line(*line);
                if (!obs.valid) continue;
                csi_field.ingest(obs);
                upsert_sensor(world, sensors, obs);
            }
        } else {
            auto now = clock::now();
            if (now >= next_synth) {
                auto obs = make_synthetic_csi(synth_tick++);
                csi_field.ingest(obs);
                upsert_sensor(world, sensors, obs);
                next_synth = now + std::chrono::milliseconds(125);
            }
        }

        const float t = static_cast<float>(world.time().simulation_time);
        const auto bodies = csi_field.bodies();
        world.entities().each<Name, Transform, Renderable>([&](EntityID, Name& name, Transform& tr, Renderable& rend){
            if (name.value=="player") {
                tr.position.x = std::sin(t*0.35f)*1.6f;
                tr.position.z = 2.2f + std::cos(t*0.35f)*0.4f;
            } else if (name.value=="npc") {
                tr.position.x = 1.4f + std::sin(t*0.55f)*0.6f;
            } else if (rend.kind=="sensor") {
                auto bit = bodies.find(name.value);
                const float e = bit==bodies.end() ? 0.f : bit->second.last.region("csi_energy");
                const bool syn = bit!=bodies.end() && bit->second.last.synthetic;
                rend.sy = 0.35f + e * 1.8f;
                rend.color = syn ? 0xc9842au : 0x2ee6a6u;
            }
        });
        world.tick(1.0/60.0);
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    hud.stop();
    std::cout << "[ok] packets=" << csi_field.packet_count()
              << " entities=" << world.entities().living_count() << "\n" << url << "\n";
    return hud_ok?0:1;
}

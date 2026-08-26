// hello-view — CSI ingest + localhost visual HUD
#include "engine/world/world.hpp"
#include "engine/fields/csi_field.hpp"
#include "engine/ingest/csi_parse.hpp"
#include "engine/ingest/jsonl_tail.hpp"
#include "engine/ingest/synthetic_csi.hpp"
#include "engine/renderer/hud_server.hpp"

#include <chrono>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

using namespace mf;

static volatile std::sig_atomic_t g_run = 1;
static void on_sig(int) { g_run = 0; }

static const char* default_jsonl() {
    const char* env = std::getenv("METAFIELD_CSI_JSONL");
    if (env && env[0]) return env;
    return "/tmp/metafield/csi.jsonl";
}

static std::string json_escape(const std::string& s) {
    std::string o;
    o.reserve(s.size());
    for (char c : s) {
        if (c == '"' || c == '\\') { o.push_back('\\'); o.push_back(c); }
        else o.push_back(c);
    }
    return o;
}

static std::string csi_array(const std::vector<float>& v) {
    std::ostringstream os;
    os << '[';
    for (size_t i = 0; i < v.size(); ++i) {
        if (i) os << ',';
        os << v[i];
    }
    os << ']';
    return os.str();
}

int main(int argc, char** argv) {
    std::signal(SIGINT, on_sig);
    std::signal(SIGTERM, on_sig);

    std::string path = default_jsonl();
    bool force_synth = false;
    int seconds = 0;
    int port = 8765;

    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--synth") force_synth = true;
        else if (a == "--file" && i + 1 < argc) path = argv[++i];
        else if (a == "--seconds" && i + 1 < argc) seconds = std::atoi(argv[++i]);
        else if (a == "--port" && i + 1 < argc) port = std::atoi(argv[++i]);
        else if (a == "--help" || a == "-h") {
            std::cout << "hello_view \n  --file PATH\n  --synth\n  --port N\n  --seconds N\n";
            return 0;
        }
    }

    World world;
    auto& csi_field = world.fields().create<CsiField>(10);
    JsonlTail tail(path, true);
    bool live = !force_synth && tail.file_exists();

    HudServer hud;
    const bool hud_ok = hud.start(port, [&]() {
        auto latest = csi_field.latest();
        auto bodies = csi_field.bodies();
        std::ostringstream os;
        os << "{\"packets\":" << csi_field.packet_count()
           << ",\"sim_t\":" << world.time().simulation_time
           << ",\"latest\":{"
           << "\"body_id\":\"" << json_escape(latest.body_id) << "\","
           << "\"synthetic\":" << (latest.synthetic ? "true" : "false") << ","
           << "\"rssi\":" << latest.region("rssi") << ","
           << "\"energy\":" << latest.region("csi_energy") << ","
           << "\"spread\":" << latest.region("csi_spread") << ","
           << "\"csi\":" << csi_array(latest.csi)
           << "},\"bodies\":[";
        bool first = true;
        for (const auto& [id, st] : bodies) {
            if (!first) os << ',';
            first = false;
            os << "{\"id\":\"" << json_escape(id)
               << "\",\"packets\":" << st.packet_count
               << ",\"synthetic\":" << (st.last.synthetic ? "true" : "false")
               << ",\"rssi\":" << st.last.region("rssi")
               << ",\"energy\":" << st.last.region("csi_energy")
               << ",\"spread\":" << st.last.region("csi_spread")
               << "}";
        }
        os << "]}";
        return os.str();
    });

    std::cout << "MetaField Engine  \xC2\xB7  visual HUD\n";
    std::cout << "  jsonl : " << path << (tail.file_exists() ? "  [present]\n" : "  [missing]\n");
    std::cout << "  mode  : " << (live ? "LIVE follow" : "synthetic (switches if jsonl appears)") << "\n";
    std::cout << "  view  : http://127.0.0.1:" << port << (hud_ok ? "\n" : "  [bind failed]\n");
    std::cout << "  stop  : Ctrl+C\n\n";

    using clock = std::chrono::steady_clock;
    const auto start = clock::now();
    auto next_synth = start;
    std::uint64_t synth_tick = 0;

    while (g_run) {
        if (seconds > 0) {
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(clock::now() - start).count();
            if (elapsed >= seconds) break;
        }
        if (!force_synth && !live && tail.file_exists()) {
            live = true;
            std::cout << "[ingest] live JSONL appeared\n";
        }
        if (live) {
            for (int n = 0; n < 64; ++n) {
                auto line = tail.poll();
                if (!line) break;
                auto obs = parse_csi_line(*line);
                if (obs.valid) csi_field.ingest(obs);
            }
        } else {
            auto now = clock::now();
            if (now >= next_synth) {
                csi_field.ingest(make_synthetic_csi(synth_tick++));
                next_synth = now + std::chrono::milliseconds(125);
            }
        }
        world.tick(1.0 / 60.0);
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    hud.stop();
    std::cout << "[ok] packets=" << csi_field.packet_count() << "\n";
    return hud_ok ? 0 : 1;
}

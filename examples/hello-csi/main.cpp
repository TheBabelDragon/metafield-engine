// hello-csi — live ESP32 CSI ingest into the MetaField World
// Follows /tmp/metafield/csi.jsonl (csi-bridge or throne-room).
// Does NOT invent packets. Does NOT bind UDP :4210.

#include "engine/world/world.hpp"
#include "engine/fields/csi_field.hpp"
#include "engine/ingest/csi_parse.hpp"
#include "engine/ingest/jsonl_tail.hpp"

#include <chrono>
#include <cmath>
#include <csignal>
#include <cstdlib>
#include <iostream>
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

static std::string bars(float v, int width = 16) {
    if (v < 0.f) v = 0.f;
    if (v > 1.f) v = 1.f;
    const int filled = static_cast<int>(std::round(v * static_cast<float>(width)));
    std::string s(static_cast<std::size_t>(width), '.');
    for (int i = 0; i < filled; ++i) s[static_cast<std::size_t>(i)] = '#';
    return s;
}

int main(int argc, char** argv) {
    std::signal(SIGINT, on_sig);
    std::signal(SIGTERM, on_sig);

    std::string path = default_jsonl();
    int seconds = 0;

    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--synth") {
            std::cerr << "--synth removed. Flash a real ESP32 CSI node.\n";
            return 2;
        } else if (a == "--file" && i + 1 < argc) path = argv[++i];
        else if (a == "--seconds" && i + 1 < argc) seconds = std::atoi(argv[++i]);
        else if (a == "--help" || a == "-h") {
            std::cout
                << "hello_csi — ingest live ESP32 CSI\n"
                << "  --file PATH     JSONL (default $METAFIELD_CSI_JSONL or /tmp/metafield/csi.jsonl)\n"
                << "  --seconds N     exit after N seconds\n";
            return 0;
        }
    }

    World world;
    auto& csi_field = world.fields().create<CsiField>(10);
    JsonlTail tail(path, true);

    std::cout << "MetaField Engine  ·  CSI ingest (hardware required)\n";
    std::cout << "  jsonl : " << path << (tail.file_exists() ? "  [present]\n" : "  [missing]\n");
    std::cout << "  mode  : WAIT for physical packets\n";
    std::cout << "  node  : flash wifi-sensing-system ESP32 / CYD, then scripts/csi-bridge.py\n\n";

    using clock = std::chrono::steady_clock;
    const auto start = clock::now();
    auto next_draw = start;
    std::uint64_t ingested = 0;

    while (g_run) {
        if (seconds > 0) {
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(clock::now() - start).count();
            if (elapsed >= seconds) break;
        }

        for (int n = 0; n < 64; ++n) {
            auto line = tail.poll();
            if (!line) break;
            auto obs = parse_csi_line(*line);
            if (!obs.valid) continue;
            if (obs.synthetic || obs.source_class == SourceClass::Synthetic) continue;
            if (obs.body_id.rfind("synthetic", 0) == 0) continue;
            csi_field.ingest(obs);
            ++ingested;
        }

        world.tick(1.0 / 60.0);
        auto now = clock::now();
        if (now >= next_draw) {
            auto latest = csi_field.latest();
            std::cout << "\033[2K\r";
            if (latest.valid && !latest.synthetic) {
                std::cout << "LIVE " << latest.body_id
                          << "  rssi " << bars(latest.region("rssi"))
                          << "  E " << bars(latest.region("csi_energy"))
                          << "  n=" << csi_field.packet_count()
                          << std::flush;
            } else {
                std::cout << "WAIT  no physical CSI yet" << std::flush;
            }
            next_draw = now + std::chrono::milliseconds(200);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    const bool pass = ingested > 0 && !csi_field.latest().synthetic;
    std::cout << "\n\n" << (pass ? "[LIVE] PASS" : "[LIVE] WAIT")
              << " ingested=" << ingested
              << " bodies=" << csi_field.body_count() << "\n";
    return pass ? 0 : 1;
}

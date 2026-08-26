// hello-csi — hands-free CSI ingest into the MetaField World
//
// Default behaviour:
//   1. If /tmp/metafield/csi.jsonl exists, follow it (live snake / throne-room).
//   2. Otherwise generate synthetic CSI at 8 Hz so the binary always does something.
//   3. If the live file appears later, automatically switch to it.
//
// Does NOT bind UDP :4210 — throne-room already owns that port.

#include "engine/world/world.hpp"
#include "engine/fields/csi_field.hpp"
#include "engine/ingest/csi_parse.hpp"
#include "engine/ingest/jsonl_tail.hpp"
#include "engine/ingest/synthetic_csi.hpp"

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
    std::string s;
    s.reserve(static_cast<size_t>(width));
    for (int i = 0; i < width; ++i) s.push_back(i < filled ? '#' : '.');
    return s;
}

static std::string spark(const std::vector<float>& csi) {
    static const char* glyphs = " .:-=+*#%@";
    std::string s;
    s.reserve(csi.size());
    for (float v : csi) {
        if (v < 0.f) v = 0.f;
        if (v > 1.f) v = 1.f;
        int idx = static_cast<int>(v * 9.f + 0.5f);
        if (idx < 0) idx = 0;
        if (idx > 9) idx = 9;
        s.push_back(glyphs[idx]);
    }
    return s;
}

int main(int argc, char** argv) {
    std::signal(SIGINT, on_sig);
    std::signal(SIGTERM, on_sig);

    std::string path = default_jsonl();
    bool force_synth = false;
    int seconds = 0;

    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--synth") force_synth = true;
        else if (a == "--file" && i + 1 < argc) path = argv[++i];
        else if (a == "--seconds" && i + 1 < argc) seconds = std::atoi(argv[++i]);
        else if (a == "--help" || a == "-h") {
            std::cout
                << "hello_csi — ingest CSI into MetaField World\n"
                << "  --file PATH     JSONL to follow (default $METAFIELD_CSI_JSONL or /tmp/metafield/csi.jsonl)\n"
                << "  --synth         force synthetic even if file exists\n"
                << "  --seconds N     exit after N seconds (default: until Ctrl+C)\n";
            return 0;
        }
    }

    World world;
    auto& csi_field = world.fields().create<CsiField>(10);

    JsonlTail tail(path, /*follow=*/true);
    bool live = !force_synth && tail.file_exists();
    bool announced_live = live;

    std::cout << "MetaField Engine  ·  CSI ingest\n";
    std::cout << "  jsonl : " << path << (tail.file_exists() ? "  [present]\n" : "  [missing]\n");
    std::cout << "  mode  : " << (live ? "LIVE follow" : "synthetic (will switch if jsonl appears)") << "\n";
    std::cout << "  port  : not bound (throne-room keeps :4210)\n";
    std::cout << "  stop  : Ctrl+C\n\n";

    using clock = std::chrono::steady_clock;
    const auto start = clock::now();
    auto next_synth = start;
    auto next_draw  = start;
    std::uint64_t synth_tick = 0;
    std::uint64_t ingested = 0;

    while (g_run) {
        if (seconds > 0) {
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(clock::now() - start).count();
            if (elapsed >= seconds) break;
        }

        if (!force_synth && !live && tail.file_exists()) {
            live = true;
            if (!announced_live) {
                std::cout << "\n[ingest] live JSONL appeared — switching off synthetic\n";
                announced_live = true;
            }
        }

        if (live) {
            for (int n = 0; n < 64; ++n) {
                auto line = tail.poll();
                if (!line) break;
                auto obs = parse_csi_line(*line);
                if (!obs.valid) continue;
                csi_field.ingest(obs);
                ++ingested;
            }
        } else {
            auto now = clock::now();
            if (now >= next_synth) {
                auto obs = make_synthetic_csi(synth_tick++);
                csi_field.ingest(obs);
                ++ingested;
                next_synth = now + std::chrono::milliseconds(125);
            }
        }

        world.tick(1.0 / 60.0);

        auto now = clock::now();
        if (now >= next_draw) {
            auto latest = csi_field.latest();
            FieldSample s = world.sample(FieldType::Electromagnetic, Vec3{0, 0, 0});
            (void)s;

            std::cout << "\033[2K\r";
            if (latest.valid) {
                std::cout
                    << (latest.synthetic ? "SYN  " : "LIVE ")
                    << latest.body_id
                    << "  rssi " << bars(latest.region("rssi"))
                    << "  E " << bars(latest.region("csi_energy"))
                    << "  spr " << bars(latest.region("csi_spread"))
                    << "  n=" << csi_field.packet_count()
                    << " bodies=" << csi_field.body_count()
                    << "  [" << spark(latest.csi) << "]"
                    << std::flush;
            } else {
                std::cout << "waiting for CSI…" << std::flush;
            }
            next_draw = now + std::chrono::milliseconds(80);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    std::cout << "\n\n[ok] ingested " << ingested
              << " packets, bodies=" << csi_field.body_count()
              << ", sim_t=" << world.time().simulation_time << " s\n";
    return 0;
}

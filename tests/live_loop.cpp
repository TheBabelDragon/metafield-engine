#include "engine/substrate/scheduler.hpp"
#include "engine/substrate/replay.hpp"
#include "engine/substrate/tick_json.hpp"
#include "engine/ingest/synthetic_csi.hpp"
#include <cmath>
#include <iostream>
#include <string>
using namespace mf;
static int fails = 0;
static void check(bool ok, const char* msg) {
    if (!ok) { std::cerr << "FAIL " << msg << "\n"; ++fails; }
    else std::cout << "ok   " << msg << "\n";
}
static VoxelField grid32() {
    VoxelField f; f.reserve_box({0,0,0},{31,0,31}); return f;
}
int main() {
    {
        auto f = grid32(); const float e0 = f.sum(Channel::Energy);
        CsiInjectSystem inj; auto obs = make_synthetic_csi(3); obs.synthetic = false; inj.bind(obs);
        FieldView view(f); FieldDeltaList out; inj.evaluate(view, out, 0.16f);
        check(std::fabs(f.sum(Channel::Energy) - e0) < 1e-8f, "input_delta: evaluate does not mutate Field");
        check(out.count_system(SYS_CSI_INPUT) > 0, "input_delta: CSI emits deltas");
        bool all_csi = true;
        for (const auto& d : out.items()) if (d.system_id != SYS_CSI_INPUT || d.channel != Channel::Energy) all_csi = false;
        check(all_csi, "input_delta: provenance is source=csi field=energy");
    }
    {
        VoxelField f; f.write({2,0,2}, Channel::Energy, 1.f);
        std::size_t n = 0; f.each_cell_sorted([&](CellCoord, const CellState&){ ++n; });
        check(n == 1, "occupied_cell: only marked cells iterate");
        check(f.exists({2,0,2}) && !f.exists({0,0,0}), "occupied_cell: exists matches mark");
    }
    {
        auto f = grid32(); f.write({16,0,16}, Channel::Temperature, 1.f);
        const float t0 = f.sum(Channel::Temperature);
        FieldScheduler s; s.add(std::make_unique<DiffusionSystem>(0.8f, Boundary::Closed, Channel::Temperature));
        for (int i = 0; i < 8; ++i) s.step(f, 0.16f);
        check(std::fabs(f.sum(Channel::Temperature) - t0) < 1e-3f, "closed_boundary_conservation: temperature");
        check(f.sample({16,0,16}, Channel::Temperature) < 1.f - 1e-4f, "diffusion_decay: heat spreads");
    }
    {
        auto f = grid32(); f.write({8,0,8}, Channel::Energy, 1.f);
        FieldScheduler s; s.add(std::make_unique<DecaySystem>(Channel::Energy, 0.5f));
        s.step(f, 0.2f);
        check(f.sample({8,0,8}, Channel::Energy) < 1.f, "diffusion_decay: energy decays");
    }
    {
        auto f = grid32(); f.write({4,0,4}, Channel::Energy, 1.f);
        for (int z=0;z<32;++z) for (int x=0;x<32;++x) f.write({x,0,z}, Channel::MomentumX, 0.25f);
        const float e0 = f.sum(Channel::Energy);
        FieldScheduler s; s.add(std::make_unique<AdvectionSystem>(Channel::Energy));
        for (int i=0;i<6;++i) s.step(f, 0.16f);
        check(std::fabs(f.sum(Channel::Energy)-e0)<1e-3f, "advection: closed energy conserved");
    }
    {
        auto live = grid32(); auto twin = grid32(); FieldTickStream stream;
        auto inj_a = std::make_unique<CsiInjectSystem>(); auto inj_b = std::make_unique<CsiInjectSystem>();
        CsiInjectSystem* pa = inj_a.get(); CsiInjectSystem* pb = inj_b.get();
        FieldScheduler live_s, copy_s;
        live_s.add(std::move(inj_a));
        live_s.add(std::make_unique<DiffusionSystem>(0.5f, Boundary::Closed, Channel::Energy));
        live_s.add(std::make_unique<DecaySystem>(Channel::Energy, 0.2f));
        live_s.add(std::make_unique<AdvectionSystem>(Channel::Energy));
        copy_s.add(std::move(inj_b));
        copy_s.add(std::make_unique<DiffusionSystem>(0.5f, Boundary::Closed, Channel::Energy));
        copy_s.add(std::make_unique<DecaySystem>(Channel::Energy, 0.2f));
        copy_s.add(std::make_unique<AdvectionSystem>(Channel::Energy));
        live_s.attach(stream);
        for (int i = 0; i < 10; ++i) {
            auto obs = make_synthetic_csi(static_cast<std::uint64_t>(i)); obs.synthetic = false;
            pa->bind(obs); pb->bind(obs);
            live_s.step(live, 0.16f); copy_s.step(twin, 0.16f);
        }
        check(fields_equivalent(live, twin), "tick_determinism: identical inputs → identical Field");
        const auto snap0 = capture(grid32());
        check(fields_equivalent(live, replay(snap0, stream.ticks())), "live_replay_equivalence: snapshot+ticks reconstruct live");
        const std::string js = tick_to_json(stream.ticks().back());
        check(js.find("\"source\":" )!=std::string::npos && js.find("\"field\":" )!=std::string::npos &&
              js.find("\"cell\":" )!=std::string::npos && js.find("\"tick\":" )!=std::string::npos,
              "fieldtick_schema: source/field/cell/tick present");
        FieldTick parsed;
        check(parse_tick_json(js, parsed) && parsed.sequence == stream.ticks().back().sequence,
              "fieldtick_schema: parse_tick_json roundtrip sequence");
        VoxelField from_json = grid32(); restore(from_json, snap0);
        for (const auto& t : stream.ticks()) {
            FieldTick p; if (!parse_tick_json(tick_to_json(t), p)) { check(false, "parse tick"); break; }
            from_json.apply(p.deltas);
        }
        check(fields_equivalent(live, from_json, 1e-4f), "live_replay_equivalence: ndjson ticks reconstruct");
    }
    if (fails) { std::cerr << fails << " live-loop test(s) failed\n"; return 1; }
    std::cout << "[OK] Live Field Loop v0.1\n"; return 0;
}

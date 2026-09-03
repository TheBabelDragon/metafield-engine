#include "engine/world/scarcity_clock.hpp"
#include "engine/world/world.hpp"
#include <cstdlib>
#include <fstream>
#include <iostream>
using namespace mf;
static int fails = 0;
static void check(bool ok, const char* msg) {
    if (!ok) { std::cerr << "FAIL " << msg << "\n"; ++fails; }
    else std::cout << "ok   " << msg << "\n";
}
int main() {
    unsetenv("METAFIELD_BTC_HEIGHT");
    unsetenv("METAFIELD_BTC_BLOCK_HASH");
    unsetenv("METAFIELD_BTC_WORK");
    unsetenv("METAFIELD_CLOCK_PATH");
    {
        auto c = resolve_clock();
        check(!c.is_anchored() && c.confidence == ClockConfidence::None, "default is unanchored");
    }
    {
        World w;
        w.time().scarcity = unanchored_clock();
        w.tick(1.0);
        w.tick(1.0);
        check(w.time().tick_count == 2, "sim ticks advance");
        check(!w.time().scarcity.btc_height.has_value(), "ticks do not invent height");
    }
    {
        setenv("METAFIELD_BTC_HEIGHT", "900001", 1);
        setenv("METAFIELD_BTC_BLOCK_HASH", "deadbeef", 1);
        auto c = resolve_clock();
        check(c.btc_height && *c.btc_height == 900001, "env height");
        check(c.confidence == ClockConfidence::Included, "env is included");
        check(!c.authoritative, "env is not confirmed");
        unsetenv("METAFIELD_BTC_HEIGHT");
        unsetenv("METAFIELD_BTC_BLOCK_HASH");
    }
    {
        auto c = parse_clock_json("{\"btc_height\":42,\"confidence\":\"confirmed\",\"source\":\"peer\"}");
        check(c.btc_height && *c.btc_height == 42, "peer height kept as evidence");
        check(c.confidence == ClockConfidence::Included, "peer cannot arrive confirmed");
    }
    {
        const char* p = "/tmp/metafield-engine-clock-test.json";
        std::ofstream(p) << "{\"btc_height\":880000,\"btc_work\":\"ff\"}";
        setenv("METAFIELD_CLOCK_PATH", p, 1);
        auto c = resolve_clock();
        check(c.btc_height && *c.btc_height == 880000, "file tip");
        check(c.source == "file", "file source");
        unsetenv("METAFIELD_CLOCK_PATH");
    }
    if (fails) { std::cerr << fails << " scarcity clock test(s) failed\n"; return 1; }
    std::cout << "[OK] scarcity clock\n";
    return 0;
}

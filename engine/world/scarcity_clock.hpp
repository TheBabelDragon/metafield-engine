#pragma once

#include "engine/ingest/json_lite.hpp"

#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>

namespace mf {

enum class ClockConfidence : uint8_t {
    None = 0,
    Included = 1,
    Confirmed = 2,
    Reorged = 3,
};

inline const char* clock_confidence_name(ClockConfidence c) {
    switch (c) {
        case ClockConfidence::Included: return "included";
        case ClockConfidence::Confirmed: return "confirmed";
        case ClockConfidence::Reorged: return "reorged";
        default: return "none";
    }
}

inline ClockConfidence clock_confidence_from(std::string_view s) {
    if (s == "included") return ClockConfidence::Included;
    if (s == "confirmed") return ClockConfidence::Confirmed;
    if (s == "reorged") return ClockConfidence::Reorged;
    return ClockConfidence::None;
}

struct ScarcityClock {
    int clock_version = 1;
    std::optional<std::int64_t> btc_height;
    std::string btc_block_hash;
    std::string btc_work;
    std::string source = "none";
    ClockConfidence confidence = ClockConfidence::None;
    bool authoritative = false;

    bool is_anchored() const {
        return btc_height.has_value() && confidence != ClockConfidence::None;
    }

    std::string to_json() const {
        std::ostringstream os;
        os << "{\"clock_version\":" << clock_version
           << ",\"epoch\":" << (btc_height ? std::to_string(*btc_height) : "null")
           << ",\"btc_height\":" << (btc_height ? std::to_string(*btc_height) : "null");
        os << ",\"btc_block_hash\":";
        if (btc_block_hash.empty()) os << "null";
        else os << "\"" << btc_block_hash << "\"";
        os << ",\"btc_work\":";
        if (btc_work.empty()) os << "null";
        else os << "\"" << btc_work << "\"";
        os << ",\"confidence\":\"" << clock_confidence_name(confidence) << "\""
           << ",\"source\":\"" << source << "\""
           << ",\"authoritative\":" << (authoritative ? "true" : "false")
           << ",\"anchored\":" << (is_anchored() ? "true" : "false")
           << "}";
        return os.str();
    }
};

inline ScarcityClock unanchored_clock() { return ScarcityClock{}; }

inline ScarcityClock clock_from_tip(std::optional<std::int64_t> height,
                                   std::string hash,
                                   std::string work,
                                   std::string source) {
    ScarcityClock c;
    if (!height.has_value()) return c;
    c.btc_height = height;
    c.btc_block_hash = std::move(hash);
    c.btc_work = std::move(work);
    c.source = source.empty() ? "explicit" : std::move(source);
    c.confidence = ClockConfidence::Included;
    c.authoritative = false;
    return c;
}

inline ScarcityClock parse_clock_json(std::string_view s) {
    if (s.empty()) return {};
    ScarcityClock c;
    if (auto h = json_lite::get_number(s, "btc_height"))
        c.btc_height = static_cast<std::int64_t>(*h);
    else if (auto h2 = json_lite::get_number(s, "height"))
        c.btc_height = static_cast<std::int64_t>(*h2);
    else if (auto h3 = json_lite::get_number(s, "epoch"))
        c.btc_height = static_cast<std::int64_t>(*h3);
    if (auto hash = json_lite::get_string(s, "btc_block_hash")) c.btc_block_hash = *hash;
    else if (auto hash2 = json_lite::get_string(s, "hash")) c.btc_block_hash = *hash2;
    if (auto work = json_lite::get_string(s, "btc_work")) c.btc_work = *work;
    else if (auto work2 = json_lite::get_string(s, "work")) c.btc_work = *work2;
    if (auto src = json_lite::get_string(s, "source")) c.source = *src;
    if (auto conf = json_lite::get_string(s, "confidence"))
        c.confidence = clock_confidence_from(*conf);
    else if (c.btc_height) c.confidence = ClockConfidence::Included;
    if (c.confidence == ClockConfidence::None) {
        c.btc_height.reset();
        c.authoritative = false;
        if (c.source.empty()) c.source = "none";
    } else if (c.source.empty()) {
        c.source = "explicit";
    }
    if (c.confidence == ClockConfidence::Confirmed &&
        (c.source == "peer" || c.source == "env" || c.source == "file" || c.source == "explicit")) {
        c.confidence = ClockConfidence::Included;
    }
    c.authoritative = false;
    return c;
}

inline ScarcityClock resolve_clock() {
    if (const char* h = std::getenv("METAFIELD_BTC_HEIGHT")) {
        char* end = nullptr;
        const long long v = std::strtoll(h, &end, 10);
        if (end != h) {
            std::string hash, work;
            if (const char* hs = std::getenv("METAFIELD_BTC_BLOCK_HASH")) hash = hs;
            if (const char* w = std::getenv("METAFIELD_BTC_WORK")) work = w;
            return clock_from_tip(static_cast<std::int64_t>(v), hash, work, "env");
        }
    }
    const char* path = std::getenv("METAFIELD_CLOCK_PATH");
    if (!path || !path[0]) path = "/tmp/metafield/btc_clock.json";
    std::ifstream in(path);
    if (in) {
        std::ostringstream ss;
        ss << in.rdbuf();
        auto c = parse_clock_json(ss.str());
        if (c.is_anchored()) {
            c.source = "file";
            c.confidence = ClockConfidence::Included;
            c.authoritative = false;
            return c;
        }
    }
    return unanchored_clock();
}

} // namespace mf

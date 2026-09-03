#include "engine/ingest/csi_parse.hpp"
#include "engine/ingest/json_lite.hpp"

#include <cmath>
#include <algorithm>
#include <cctype>

namespace mf {

namespace {

float cl01(float v) {
    return std::max(0.f, std::min(1.f, v));
}

float rssi_norm(float rssi_dbm) {
    return cl01((rssi_dbm + 90.f) / 60.f);
}

void derive_regions(FieldObservation& o) {
    const auto& csi = o.csi;
    float mean = 0.f, peak = 0.f, energy = 0.f, spread = 0.f;
    if (!csi.empty()) {
        float sum = 0.f, sq = 0.f;
        peak = csi[0];
        for (float v : csi) {
            sum += v;
            sq += v * v;
            if (v > peak) peak = v;
        }
        mean = sum / static_cast<float>(csi.size());
        energy = std::sqrt(sq / static_cast<float>(csi.size()));
        if (csi.size() > 1) {
            float var = 0.f;
            for (float v : csi) {
                float d = v - mean;
                var += d * d;
            }
            spread = std::sqrt(var / static_cast<float>(csi.size()));
        }
    }
    auto push = [&](const char* name, float value, float conf) {
        o.regions.push_back(FieldRegion{name, cl01(value), conf});
    };
    push("rssi",       rssi_norm(o.rssi_dbm), 1.00f);
    push("csi_mean",   mean,                  0.95f);
    push("csi_peak",   peak,                  0.95f);
    push("csi_energy", energy,                0.90f);
    push("csi_spread", spread * 2.f,          0.85f);
}

void take_scalar(FieldObservation& o, std::string_view line, const char* key) {
    if (auto v = json_lite::get_number(line, key))
        o.regions.push_back(FieldRegion{key, static_cast<float>(*v), 1.f});
}

} // namespace

FieldObservation parse_csi_line(std::string_view line) {
    FieldObservation o;
    while (!line.empty() && std::isspace(static_cast<unsigned char>(line.front())))
        line.remove_prefix(1);
    if (line.empty() || line.front() != '{') return o;

    auto type = json_lite::get_string(line, "type");
    auto node = json_lite::get_string(line, "node");
    auto body = json_lite::get_string(line, "body_id");
    auto body_type = json_lite::get_string(line, "body_type");
    const bool c3 = (type && *type == "c3_swarm") || (body_type && *body_type == "c3_swarm");

    const bool looks_wifi =
        (type && *type == "wifi_csi") ||
        (node && line.find("\"csi\"") != std::string_view::npos) ||
        (body_type && *body_type == "wifi_csi") ||
        (line.find("\"field_regions\"") != std::string_view::npos && body);

    if (!looks_wifi && !c3 && !node && !body) return o;

    o.body_id = body ? *body : (node ? *node : (c3 ? "c3-unknown" : "csi-unknown"));
    o.body_type = body_type ? *body_type : (c3 ? "c3_swarm" : "wifi_csi");
    if (auto ts = json_lite::get_string(line, "timestamp")) {
        o.timestamp = *ts;
    } else if (auto tn = json_lite::get_number(line, "timestamp")) {
        o.timestamp = std::to_string(*tn);
    }
    o.clock = parse_clock_json(line);
    if (auto sc = json_lite::get_string(line, "source_class"))
        o.source_class = source_class_from(*sc);
    else if (auto src = json_lite::get_string(line, "source"))
        o.source_class = source_class_from(*src);
    if (auto inst = json_lite::get_string(line, "instrument")) o.instrument = *inst;
    if (line.find("\"synthetic\":true") != std::string_view::npos) o.synthetic = true;

    if (auto rssi = json_lite::get_number(line, "rssi")) {
        o.rssi_dbm = static_cast<float>(*rssi);
    } else if (auto rssi2 = json_lite::get_number(line, "rssi_dbm")) {
        o.rssi_dbm = static_cast<float>(*rssi2);
    }

    o.csi = json_lite::get_array_f(line, "csi");

    if (line.find("\"field_regions\"") != std::string_view::npos) {
        size_t pos = 0;
        while (true) {
            auto rkey = line.find("\"region\"", pos);
            if (rkey == std::string_view::npos) break;
            auto name = json_lite::get_string(line.substr(rkey), "region");
            auto obs  = json_lite::get_number(line.substr(rkey), "observed");
            auto conf = json_lite::get_number(line.substr(rkey), "confidence");
            if (name && obs) {
                o.regions.push_back(FieldRegion{
                    *name,
                    static_cast<float>(*obs),
                    conf ? static_cast<float>(*conf) : 1.f
                });
            }
            pos = rkey + 8;
        }
    }

    if (c3) {
        take_scalar(o, line, "temperature");
        take_scalar(o, line, "information");
        take_scalar(o, line, "energy");
        take_scalar(o, line, "signal");
    }

    if (o.regions.empty()) {
        derive_regions(o);
    }

    o.valid = !o.body_id.empty();
    if (o.source_class == SourceClass::Synthetic) o.synthetic = true;
    if (o.source_class == SourceClass::Unknown)
        o.source_class = o.synthetic ? SourceClass::Synthetic : SourceClass::Physical;
    return o;
}

} // namespace mf

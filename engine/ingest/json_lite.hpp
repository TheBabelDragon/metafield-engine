#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <cctype>
#include <cstdlib>

namespace mf {
namespace json_lite {

inline void skip_ws(std::string_view s, size_t& i) {
    while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i]))) ++i;
}

inline std::optional<size_t> find_key(std::string_view s, std::string_view key) {
    std::string quoted;
    quoted.reserve(key.size() + 2);
    quoted.push_back('"');
    quoted.append(key);
    quoted.push_back('"');

    size_t pos = 0;
    while (true) {
        auto found = s.find(quoted, pos);
        if (found == std::string_view::npos) return std::nullopt;
        size_t after = found + quoted.size();
        skip_ws(s, after);
        if (after < s.size() && s[after] == ':') return after + 1;
        pos = found + 1;
    }
}

inline std::optional<std::string> get_string(std::string_view s, std::string_view key) {
    auto start = find_key(s, key);
    if (!start) return std::nullopt;
    size_t i = *start;
    skip_ws(s, i);
    if (i >= s.size() || s[i] != '"') return std::nullopt;
    ++i;
    std::string out;
    while (i < s.size() && s[i] != '"') {
        if (s[i] == '\\' && i + 1 < s.size()) {
            out.push_back(s[i + 1]);
            i += 2;
            continue;
        }
        out.push_back(s[i++]);
    }
    return out;
}

inline std::optional<double> get_number(std::string_view s, std::string_view key) {
    auto start = find_key(s, key);
    if (!start) return std::nullopt;
    size_t i = *start;
    skip_ws(s, i);
    if (i >= s.size()) return std::nullopt;
    if (s.compare(i, 4, "null") == 0) return std::nullopt;
    char* end = nullptr;
    const char* begin = s.data() + i;
    double v = std::strtod(begin, &end);
    if (end == begin) return std::nullopt;
    return v;
}

inline std::vector<float> get_array_f(std::string_view s, std::string_view key) {
    auto start = find_key(s, key);
    if (!start) return {};
    size_t i = *start;
    skip_ws(s, i);
    if (i >= s.size() || s[i] != '[') return {};
    ++i;
    std::vector<float> out;
    out.reserve(32);
    while (i < s.size() && s[i] != ']') {
        skip_ws(s, i);
        if (i < s.size() && s[i] == ']') break;
        if (i < s.size() && s[i] == ',') { ++i; continue; }
        char* end = nullptr;
        const char* begin = s.data() + i;
        double v = std::strtod(begin, &end);
        if (end == begin) break;
        out.push_back(static_cast<float>(v));
        i = static_cast<size_t>(end - s.data());
        skip_ws(s, i);
        if (i < s.size() && s[i] == ',') ++i;
    }
    return out;
}

} // namespace json_lite
} // namespace mf

#pragma once

#include <fstream>
#include <string>
#include <optional>
#include <cstdint>
#include <sys/stat.h>

namespace mf {

class JsonlTail {
public:
    explicit JsonlTail(std::string path, bool follow = true, std::int64_t lookback = 65536)
        : path_(std::move(path)), follow_(follow), lookback_(lookback) {}

    const std::string& path() const { return path_; }

    bool file_exists() const {
        struct stat st{};
        return ::stat(path_.c_str(), &st) == 0 && S_ISREG(st.st_mode);
    }

    std::int64_t bytes() const { return file_size(); }

    std::optional<std::string> poll() {
        if (!stream_.is_open()) {
            stream_.open(path_, std::ios::in);
            if (!stream_.is_open()) return std::nullopt;
            const auto sz = file_size();
            if (lookback_ <= 0) {
                stream_.seekg(0, std::ios::end);
                pos_ = sz;
            } else if (follow_ && sz > lookback_) {
                stream_.seekg(sz - lookback_, std::ios::beg);
                std::string discard;
                std::getline(stream_, discard);
                auto g = stream_.tellg();
                pos_ = g >= 0 ? static_cast<std::int64_t>(g) : sz;
            } else {
                pos_ = 0;
            }
            last_size_ = sz;
        }

        std::string line;
        if (std::getline(stream_, line)) {
            auto g = stream_.tellg();
            if (g >= 0) pos_ = static_cast<std::int64_t>(g);
            if (!line.empty() && line.back() == '\r') line.pop_back();
            return line;
        }

        stream_.clear();
        const auto sz = file_size();
        if (sz < last_size_) {
            stream_.close();
            pos_ = 0;
            last_size_ = sz;
            return std::nullopt;
        }
        if (sz > pos_) stream_.seekg(pos_, std::ios::beg);
        else {
            stream_.seekg(0, std::ios::end);
            pos_ = sz;
        }
        last_size_ = sz;
        return std::nullopt;
    }

private:
    std::int64_t file_size() const {
        struct stat st{};
        if (::stat(path_.c_str(), &st) != 0) return 0;
        return static_cast<std::int64_t>(st.st_size);
    }

    std::string path_;
    bool follow_ = true;
    std::int64_t lookback_ = 65536;
    std::ifstream stream_;
    std::int64_t last_size_ = 0;
    std::int64_t pos_ = 0;
};

} // namespace mf

#pragma once

#include "engine/renderer/hud_page.hpp"

#include <atomic>
#include <string>
#include <thread>
#include <functional>
#include <cstring>
#include <cstdio>
#include <cstdint>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

namespace mf {

class HudServer {
public:
    using StateFn = std::function<std::string()>;

    bool start(int port, StateFn state) {
        state_ = std::move(state);
        port_ = port;
        fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
        if (fd_ < 0) return false;
        int yes = 1;
        setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(static_cast<uint16_t>(port));
        inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
        if (bind(fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
            ::close(fd_);
            fd_ = -1;
            return false;
        }
        if (listen(fd_, 16) < 0) {
            ::close(fd_);
            fd_ = -1;
            return false;
        }
        run_ = true;
        th_ = std::thread([this]{ loop(); });
        return true;
    }

    void stop() {
        run_ = false;
        if (fd_ >= 0) {
            int fd = fd_;
            fd_ = -1;
            shutdown(fd, SHUT_RDWR);
            ::close(fd);
        }
        if (th_.joinable()) th_.join();
    }

    ~HudServer() { stop(); }
    int port() const { return port_; }

private:
    void loop() {
        while (run_) {
            sockaddr_in cli{};
            socklen_t n = sizeof(cli);
            int c = accept(fd_, reinterpret_cast<sockaddr*>(&cli), &n);
            if (c < 0) continue;
            handle(c);
            ::close(c);
        }
    }

    void handle(int c) {
        char buf[2048];
        const ssize_t n = ::recv(c, buf, sizeof(buf) - 1, 0);
        if (n <= 0) return;
        buf[n] = 0;
        const bool want_state = std::strstr(buf, "GET /state") != nullptr;
        const std::string body = want_state ? (state_ ? state_() : "{}") : std::string(HUD_PAGE);
        const char* type = want_state ? "application/json" : "text/html; charset=utf-8";
        char hdr[256];
        const int hlen = std::snprintf(
            hdr, sizeof(hdr),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: %s\r\n"
            "Content-Length: %zu\r\n"
            "Cache-Control: no-store\r\n"
            "Connection: close\r\n"
            "\r\n",
            type, body.size());
        ::send(c, hdr, static_cast<size_t>(hlen), 0);
        ::send(c, body.data(), body.size(), 0);
    }

    StateFn state_;
    std::thread th_;
    std::atomic<bool> run_{false};
    int fd_ = -1;
    int port_ = 8765;
};

} // namespace mf

#pragma once

#include <atomic>
#include <string>
#include <thread>
#include <functional>
#include <fstream>
#include <sstream>
#include <cstring>
#include <cstdio>
#include <cstdint>
#include <vector>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

namespace mf {

inline std::string load_hud_page() {
    const char* candidates[] = {
        "engine/renderer/hud_page.html",
        "../engine/renderer/hud_page.html",
        "../../engine/renderer/hud_page.html",
        "hud_page.html",
    };
    for (const char* p : candidates) {
        std::ifstream in(p);
        if (!in) continue;
        std::ostringstream ss;
        ss << in.rdbuf();
        auto s = ss.str();
        if (!s.empty()) return s;
    }
    return "<!doctype html><meta charset=utf-8><title>MetaField</title>"
           "<body style='background:#111;color:#9f9;font-family:sans-serif'>"
           "<h1>MetaField Engine</h1><p>HUD page file not found.</p>"
           "<pre id=s></pre><script>async function t(){s.textContent=await (await fetch('/state')).text()}"
           "setInterval(t,200);t()</script>";
}

class HudServer {
public:
    using StateFn = std::function<std::string()>;

    bool start(int port, StateFn state) {
        state_ = std::move(state);
        page_ = load_hud_page();
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
            ::close(fd_); fd_ = -1; return false;
        }
        if (listen(fd_, 16) < 0) {
            ::close(fd_); fd_ = -1; return false;
        }
        run_ = true;
        th_ = std::thread([this]{ loop(); });
        return true;
    }

    void stop() {
        run_ = false;
        if (fd_ >= 0) {
            int fd = fd_; fd_ = -1;
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
        const std::string& body = want_state ? scratch_ : page_;
        std::string state;
        const std::string* send = &page_;
        if (want_state) {
            state = state_ ? state_() : "{}";
            send = &state;
        }
        const char* type = want_state ? "application/json" : "text/html; charset=utf-8";
        char hdr[256];
        const int hlen = std::snprintf(
            hdr, sizeof(hdr),
            "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %zu\r\n"
            "Cache-Control: no-store\r\nConnection: close\r\n\r\n",
            type, send->size());
        ::send(c, hdr, static_cast<size_t>(hlen), 0);
        ::send(c, send->data(), send->size(), 0);
        (void)body;
    }

    StateFn state_;
    std::string page_;
    std::string scratch_;
    std::thread th_;
    std::atomic<bool> run_{false};
    int fd_ = -1;
    int port_ = 8765;
};

} // namespace mf

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
    std::string s;
    for (const char* p : candidates) {
        std::ifstream in(p);
        if (!in) continue;
        std::ostringstream ss;
        ss << in.rdbuf();
        s = ss.str();
        if (!s.empty()) break;
    }
    if (s.empty())
        s = "<!doctype html><meta charset=utf-8><title>MetaField</title><p>HUD page file not found.</p>";
    if (s.find("move=up") == std::string::npos) {
        s += "<script>"
             "(function(){"
             "const keymap={ArrowUp:'up',ArrowDown:'down',ArrowLeft:'left',ArrowRight:'right',"
             "w:'up',a:'left',s:'down',d:'right',W:'up',A:'left',S:'down',D:'right'};"
             "addEventListener('keydown',function(e){"
             "const m=keymap[e.key]; if(!m)return; e.preventDefault();"
             "fetch('/mode?move='+m,{cache:'no-store'});"
             "});"
             "})();</script>";
    }
    return s;
}

class HudServer {
public:
    using StateFn = std::function<std::string()>;
    using CmdFn   = std::function<std::string(std::string_view path)>;

    bool start(int port, StateFn state, CmdFn cmd = {}) {
        state_ = std::move(state);
        cmd_ = std::move(cmd);
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
        std::string path = "/";
        if (const char* sp = std::strchr(buf, ' ')) {
            const char* start = sp + 1;
            const char* end = std::strchr(start, ' ');
            if (end) path.assign(start, end);
            else path = start;
        }
        std::string payload;
        const char* type = "text/html; charset=utf-8";
        if (path.rfind("/state", 0) == 0) {
            payload = state_ ? state_() : "{}";
            type = "application/json";
        } else if (path.rfind("/mode", 0) == 0) {
            payload = cmd_ ? cmd_(path) : "{\"ok\":false}";
            type = "application/json";
        } else {
            payload = page_;
        }
        char hdr[256];
        const int hlen = std::snprintf(
            hdr, sizeof(hdr),
            "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %zu\r\n"
            "Cache-Control: no-store\r\nAccess-Control-Allow-Origin: *\r\n"
            "Connection: close\r\n\r\n",
            type, payload.size());
        ::send(c, hdr, static_cast<size_t>(hlen), 0);
        ::send(c, payload.data(), payload.size(), 0);
    }

    StateFn state_;
    CmdFn cmd_;
    std::string page_;
    std::thread th_;
    std::atomic<bool> run_{false};
    int fd_ = -1;
    int port_ = 8765;
};

} // namespace mf

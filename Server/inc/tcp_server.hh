#pragma once
#include <socket_options.hh>

#include <cstdint>
#include <netinet/in.h>
#include <string_view>
#include <vector>

class TcpServer {
    private:
    static constexpr std::string_view LOG_TAG = "TcpServer";

    static constexpr uint16_t
        BIND_PORT = 8080,
        TIMEOUT_SEC = 3;

    static constexpr SocketOptions::KeepAliveOptions KEEPALIVE_OPTIONS = {
        .enabled = 1,
        .idleTime = 20,
        .interval = 12,
        .probes = 10,
    };

    int fd_;
    std::vector<int> clients;

    public:
    TcpServer();
    TcpServer(const TcpServer&) = delete;
    TcpServer& operator=(const TcpServer&) = delete;

    TcpServer(TcpServer&&);
    TcpServer& operator=(TcpServer&&);

    bool open();

    void update(bool checkWritable);

    void close();
};

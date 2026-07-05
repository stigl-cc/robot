#pragma once
#include <socket_options.hh>

#include <cstdint>
#include <netinet/in.h>
#include <sys/poll.h>

class TcpServer {
    private:
    static constexpr std::string_view LOG_TAG = "TcpServer";

    static constexpr uint16_t
        TIMEOUT_SEC = 3,
        BIND_PORT = 8080;

    static constexpr SocketOptions::KeepAliveOptions KA_OPTIONS = {
        .enabled = 1,
        .idleTime = 20,
        .interval = 12,
        .probes = 10,
    };

    int fd_;

    public:
    TcpServer();
    TcpServer(const TcpServer&) = delete;
    TcpServer& operator=(const TcpServer&) = delete;

    TcpServer(TcpServer&&);
    TcpServer& operator=(TcpServer&&);

    bool open();

    void close();
};

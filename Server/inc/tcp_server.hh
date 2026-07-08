#pragma once
#include <optional>
#include <packet.hh>
#include <queue>
#include <socket_options.hh>

#include <cstdint>
#include <netinet/in.h>
#include <string_view>

/*
  We will only ever handle 1 truly active connection.
 */

class TcpServer {
    private:
    static constexpr std::string_view LOG_TAG = "TcpServer";

    static constexpr uint16_t
        BIND_PORT = 8080,
        TIMEOUT_SEC = 3;

    static constexpr size_t BUFFER_LEN = 8192;

    static constexpr SocketOptions::KeepAliveOptions KEEPALIVE_OPTIONS = {
        .enabled = 1,
        .idleTime = 20,
        .interval = 12,
        .probes = 10,
    };

    int fd_;
    int client_;

    uint8_t buffer_[BUFFER_LEN];

    TcpRecvPacket recvPacket_;
    std::queue<TcpSendPacket> sendPacketQueue_;

    void handlePollServer(int revents);
    void handlePollClient(int revents);
    void closeClient();

    public:
    TcpServer();
    TcpServer(const TcpServer&) = delete;
    TcpServer& operator=(const TcpServer&) = delete;

    TcpServer(TcpServer&&);
    TcpServer& operator=(TcpServer&&);

    bool open();
    void update();
    void close();

    ~TcpServer();
};

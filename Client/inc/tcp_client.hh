#pragma once
#include <socket_options.hh>
#include <packet.hh>
#include <callback.hh>

#include <queue>
#include <cstdint>
#include <netinet/in.h>
#include <string_view>

class TcpClient {
    public:
    enum class Status {
        Connecting,
        Connected,
        Failed,
        Closed,
    };

    private:
    static constexpr std::string_view LOG_TAG = "TcpClient";

    static constexpr uint16_t
        BIND_PORT = 1231,
        TIMEOUT_SEC = 10,
        MAX_RECONNECTS = 5;

    static constexpr uint32_t
        BUFFER_LEN = 8192;

    static constexpr SocketOptions::KeepAliveOptions KEEPALIVE_OPTIONS = {
        .enabled = 1,
        .idleTime = 20,
        .interval = 12,
        .probes = 10,
    };

    int fd_;
    uint8_t buffer_[BUFFER_LEN];

    TcpRecvPacket recvPacket_;
    EventInvoker<const TcpRecvPacket> recvEventInvoker_;

    std::queue<TcpSendPacket> sendPacketQueue_;

    enum Status status_;
    struct sockaddr_in serverAddress_;
    int reconnectCounter_ = 0;

    bool handlePollin();
    bool handlePollout();
    void handlePollerr();

    bool connect();
    bool reconnect();

    public:
    TcpClient(sockaddr_in serverAddress);
    TcpClient(std::string ip, uint16_t port);
    TcpClient(const TcpClient&) = delete;
    TcpClient& operator=(const TcpClient&) = delete;

    TcpClient(TcpClient&&);
    TcpClient& operator=(TcpClient&&);

    bool open();

    void send(const TcpSendPacket& packet);

    IEvent<const TcpRecvPacket>& getRecvEvent();

    Status getStatus() const;
    void update();

    void close();

    ~TcpClient();
};

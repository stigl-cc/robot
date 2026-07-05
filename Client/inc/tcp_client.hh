#pragma once

#include <cstdint>
#include <netinet/in.h>
#include <string_view>
#include <sys/poll.h>

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
        RECV_BUFFER_LEN = 8192;

    int fd_;
    uint8_t recvBuffer_[RECV_BUFFER_LEN];

    uint8_t* recvPacketBuffer_ = nullptr;
    size_t
        recvPacketLength_ = 0,
        recvPacketPosition_ = 0;

    enum Status status_;
    struct sockaddr_in serverAddress_;
    int reconnectCounter_ = 0;
    bool isWritable_;

    bool connect();
    bool reconnect();

    bool setTimeout(int fd, uint32_t msTimeout);

    public:
    TcpClient(sockaddr_in server);
    TcpClient(const TcpClient&) = delete;
    TcpClient& operator=(const TcpClient&) = delete;

    TcpClient(TcpClient&&);
    TcpClient& operator=(TcpClient&&);

    bool open();

    Status getStatus() const;
    bool isWritable() const;
    void update(bool checkWritable);

    void close();

    ~TcpClient();
};

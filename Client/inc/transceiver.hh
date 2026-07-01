#pragma once

#include <cstdint>
#include <netinet/in.h>
#include <sys/poll.h>

class Transceiver {
    public:
    enum class Status {
        Connecting,
        Connected,
        Failed,
        Closed,
    };

    private:
    const char *LOG_TAG = "Transceiver";

    static constexpr uint16_t
        BIND_PORT = 1231,
        TIMEOUT_S = 10,
        MAX_RECONNECTS = 5;

    static constexpr uint32_t
        RECV_BUFFER_LEN = 8192;

    int fd_;
    uint8_t recvBuffer_[RECV_BUFFER_LEN];

    enum Status status_;
    sockaddr_in serverAddress_;
    int reconnectCounter_ = 0;

    bool connect();
    bool reconnect();

    public:
    Transceiver(sockaddr_in server);

    bool open();

    Status getStatus();
    void update(bool checkWritable);

    void close();

    ~Transceiver();
};

#pragma once

#include <cstdint>
#include <netinet/in.h>
#include <sys/poll.h>

class Transceiver {
    protected:
    const char* log_tag = "Transceiver: ";

    static constexpr uint16_t
        bind_port = 1231,
        timeout_s = 10,
        max_reconnects = 5;
    static constexpr uint32_t
        recv_buffer_len = 8192;

    int fd;
    uint8_t buffer[recv_buffer_len];

    void UpdateEvents();

    short handlePoll(pollfd fds);
    bool isWritable();
    void onDataAvilable();

    public:
    Transceiver();
    ~Transceiver();

    bool Connect(sockaddr_in);
    bool Open(sockaddr_in server);
    void Close();
};

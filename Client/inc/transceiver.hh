#pragma once

#include <cstdint>
#include <netinet/in.h>

class Transceiver {
    protected:
    const char* log_tag = "Transceiver: ";

    static constexpr uint16_t
        bind_port = 1231,
        timeout_s = 10,
        max_reconnects = 5;

    int fd;

    void UpdateEvents();

    public:
    Transceiver();
    ~Transceiver();

    bool Connect(sockaddr_in);
    bool Open(sockaddr_in server);
    void Close();
};

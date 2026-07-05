#pragma once

#include <cstdint>
#include <string_view>

class SocketOptions {
    private:
    static constexpr std::string_view LOG_TAG = "SocketOptions";
    public:
    struct KeepAliveOptions {
        int32_t
            enabled,
            idleTime,
            interval,
            probes;
    };

    static bool setTimeout(int fd, uint32_t msTimeout);
    static bool setKeepAlive(int fd, KeepAliveOptions options);
};

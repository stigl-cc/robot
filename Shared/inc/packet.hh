#pragma once

#include <cstdint>
#include <span>
#include <sys/types.h>
#include <vector>

class TcpPacket {
    private:
    uint8_t headerBytesWritten_;
    uint32_t totalLength_;
    std::vector<uint8_t> contents_;

    public:
    TcpPacket();
    size_t write(const std::span<uint8_t>& data);

    ssize_t getTotalLength() const;
    ssize_t getLeftToWrite() const;

    bool isHeaderComplete() const;
    bool isPacketComplete() const;

    const std::vector<uint8_t>& getBuffer() const;
};

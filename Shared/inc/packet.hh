#pragma once

#include <cstdint>
#include <span>
#include <sys/types.h>
#include <vector>

// Constructing the buffer by parts, then the buffer can be extracted
class TcpRecvPacket {
    private:
    uint8_t headerBytesWritten_;
    uint32_t totalLength_;
    std::vector<uint8_t> contents_;

    public:
    TcpRecvPacket();
    size_t write(const std::span<uint8_t>& data);

    ssize_t getTotalLength() const;
    ssize_t getLeftToWrite() const;

    bool isHeaderComplete() const;
    bool isPacketComplete() const;

    const std::vector<uint8_t>& getBuffer() const;
};

// Buffer provided instantly, then it is extracted by parts
class TcpSendPacket {
    private:
    uint32_t readLength_;
    std::vector<uint8_t> contents_;

    public:
    TcpSendPacket(const std::span<uint8_t>& data);

    size_t read(uint8_t* buffer, size_t size);
    void advance(size_t size);

    size_t getLeftToRead() const;
    bool isPacketRead() const;
};

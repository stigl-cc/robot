#include <packet.hh>

#include <cstring>

TcpRecvPacket::TcpRecvPacket() : headerBytesWritten_(0), totalLength_(0), contents_() { }

ssize_t TcpRecvPacket::getLeftToWrite() const {
    if(!isHeaderComplete())
        return -1;

    return totalLength_ - contents_.size();
}

ssize_t TcpRecvPacket::getTotalLength() const {
    if(!isHeaderComplete())
        return -1;

    return totalLength_;
}

bool TcpRecvPacket::isHeaderComplete() const {
    return headerBytesWritten_ >= sizeof(totalLength_);
}

bool TcpRecvPacket::isPacketComplete() const {
    return isHeaderComplete() && contents_.size() >= totalLength_;
}

size_t TcpRecvPacket::write(const std::span<uint8_t>& data) {
    size_t data_pos = 0, bytes_to_insert = 0;
    if(!isHeaderComplete()) {
        while(headerBytesWritten_ < sizeof(totalLength_) && data_pos < data.size()) {
            totalLength_ = totalLength_ << 8 | data[data_pos++];
            headerBytesWritten_++;
        }

        if(isHeaderComplete()) {
            totalLength_ = le32toh(totalLength_);
            contents_.reserve(totalLength_);
        }
    }

    if(isHeaderComplete()) {
        bytes_to_insert = std::min<size_t>(getLeftToWrite(), data.size() - data_pos);
        contents_.insert(contents_.end(), data.begin() + data_pos, data.begin() + (data_pos + bytes_to_insert));
    }

    return data_pos + bytes_to_insert;
}

const std::vector<uint8_t>& TcpRecvPacket::getBuffer() const {
    return contents_;
}

TcpSendPacket::TcpSendPacket(const std::span<uint8_t>& data) : contents_() {
    uint32_t total_length = htole32(static_cast<uint32_t>(data.size()));
    contents_.reserve(sizeof(total_length) + data.size());

    const uint8_t* total_length_bytes = reinterpret_cast<const uint8_t*>(&total_length);
    contents_.insert(contents_.end(), total_length_bytes, total_length_bytes + sizeof(total_length));

    contents_.insert(contents_.end(), data.begin(), data.end());
}

size_t TcpSendPacket::read(uint8_t* buffer, size_t size) {
    size = std::min(contents_.size() - readLength_, size);
    memcpy(buffer, contents_.data(), size);
    return size;
}

void TcpSendPacket::advance(size_t size) {
    readLength_ += size;
}

size_t TcpSendPacket::getLeftToRead() const {
    return contents_.size() - readLength_;
}

bool TcpSendPacket::isPacketRead() const {
    return readLength_ >= contents_.size();
}

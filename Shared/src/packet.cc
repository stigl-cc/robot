#include <packet.hh>

TcpPacket::TcpPacket() : headerBytesWritten_(0), totalLength_(-1), contents_() { }

ssize_t TcpPacket::getLeftToWrite() const {
    if(!isHeaderComplete())
        return -1;

    return totalLength_ - contents_.size();
}

ssize_t TcpPacket::getTotalLength() const {
    if(!isHeaderComplete())
        return -1;

    return totalLength_;
}

bool TcpPacket::isHeaderComplete() const {
    return headerBytesWritten_ >= sizeof(headerBytesWritten_);
}

bool TcpPacket::isPacketComplete() const {
    return contents_.size() >= totalLength_;
}

size_t TcpPacket::write(const std::span<uint8_t>& data) {
    size_t data_pos = 0, bytes_to_insert = 0;
    if(!isHeaderComplete()) {
        while(headerBytesWritten_ < sizeof(headerBytesWritten_) && data_pos < data.size())
            totalLength_ = totalLength_ << 8 | data[data_pos++];

        if(isHeaderComplete())
            contents_.reserve(totalLength_);
    }

    if(isHeaderComplete()) {
        bytes_to_insert = std::min<size_t>(getLeftToWrite(), data.size());
        contents_.insert(contents_.end(), data.begin() + data_pos, data.begin() + (data_pos + bytes_to_insert));
    }

    return data_pos + bytes_to_insert;
}

const std::vector<uint8_t>& TcpPacket::getBuffer() const {
    return contents_;
}

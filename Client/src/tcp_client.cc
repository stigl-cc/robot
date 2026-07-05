#include "socket_options.hh"
#include <tcp_client.hh>
#include <logger.hh>

#include <algorithm>
#include <sys/poll.h>
#include <cerrno>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <poll.h>
#include <utility>

bool TcpClient::connect() {
    int ret = ::connect(fd_, reinterpret_cast<sockaddr*>(&serverAddress_), sizeof(serverAddress_));

    if(ret >= 0 || errno == EINPROGRESS) {
        status_ = Status::Connecting;
        return true;
    }

    log_tag_no(LOG_ERR, "connect");
    status_ = Status::Failed;
    return false;
}

bool TcpClient::reconnect() {
    if(reconnectCounter_++ < MAX_RECONNECTS) {
        log_tag(LOG_INFO, "Attempting to reconnect...");
        close();
        return open();
    }
    return false;
}

TcpClient::TcpClient(sockaddr_in serverAddress)
    : fd_(-1), serverAddress_(serverAddress) {}

TcpClient::TcpClient(TcpClient&& other) {
    fd_ = std::exchange(other.fd_, -1);
    memcpy(recvBuffer_, other.recvBuffer_, RECV_BUFFER_LEN);
    recvPacketBuffer_ = std::exchange(other.recvPacketBuffer_, nullptr);
    recvPacketLength_ = std::exchange(other.recvPacketLength_, 0);
    recvPacketPosition_ = std::exchange(other.recvPacketPosition_, 0);
    status_ = std::exchange(other.status_, Status::Failed);
    serverAddress_ = std::exchange(other.serverAddress_, {});
    reconnectCounter_ = std::exchange(other.reconnectCounter_, 0);
    isWritable_ = std::exchange(other.isWritable_, false);
}

TcpClient& TcpClient::operator=(TcpClient&& other) {
    if(&other == this)
        return *this;

    if(fd_ >= 0) close();
    fd_ = std::exchange(other.fd_, -1);
    memcpy(recvBuffer_, other.recvBuffer_, RECV_BUFFER_LEN);
    recvPacketBuffer_ = std::exchange(other.recvPacketBuffer_, nullptr);
    recvPacketLength_ = std::exchange(other.recvPacketLength_, 0);
    recvPacketPosition_ = std::exchange(other.recvPacketPosition_, 0);
    status_ = std::exchange(other.status_, Status::Failed);
    serverAddress_ = std::exchange(other.serverAddress_, {});
    reconnectCounter_ = std::exchange(other.reconnectCounter_, 0);
    isWritable_ = std::exchange(other.isWritable_, false);

    return *this;
}

bool TcpClient::open() {
    if((fd_ = socket(AF_INET, SOCK_STREAM, 0)) == 1) {
        log_tag_no(LOG_ERR, "open socket");
        status_ = Status::Failed;
        return false;
    }

    if(fcntl(fd_, F_SETFL, O_NONBLOCK) == -1) {
        log_tag_no(LOG_ERR, "set O_NONBLOCK");
        close();
        status_ = Status::Failed;
        return false;
    }

    int reuse_addr = 0b1;
    if(setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(reuse_addr)) == -1) {
        log_tag_no(LOG_WARN, "set SO_REUSEADDR");
    }

    if(!SocketOptions::setTimeout(fd_, TIMEOUT_SEC * 1'000)) {
        log_tag_no(LOG_WARN, "set Timeout");
    }

    if(!SocketOptions::setKeepAlive(fd_, KEEPALIVE_OPTIONS)) {
        log_tag(LOG_WARN, "set up KeepAlive");
    }

    sockaddr_in bind_address = {
        .sin_family = AF_INET,
        .sin_port = htons(BIND_PORT),
        .sin_addr = INADDR_ANY,
    };
    if(bind(fd_, reinterpret_cast<sockaddr*>(&bind_address), sizeof(bind_address)) == -1) {
        log_tag_no(LOG_WARN, "bind");
    }

    return connect();
}

TcpClient::Status TcpClient::getStatus() const {
    return status_;
}

bool TcpClient::isWritable() const {
    return isWritable_;
}

/*
  Check according to these:

  State               POLLIN POLLOUT POLLHUP / POLLERR   Secondary Check
  Connection Success  Don't  Yes     No             getsockopt() returns 0
  Connection Failed   Don't  Don't   Yes            getsockopt() returns error
  Incoming Data       Yes    Don't   No             recv() returns > 0
  Clean Disconnect    Yes    Don't   Don't          recv() returns 0
  Broken Connection   Don't  Don't   Yes            recv() returns -1 (ECONNRESET)
*/

void TcpClient::update(bool checkWritable) {
    int ret;

    short checkPollout = (status_ == Status::Connecting || checkWritable) ? POLLOUT : 0;
    pollfd fds = {
        .fd = fd_,
        .events = static_cast<short>(POLLIN | checkPollout),
    };

    ret = poll(&fds, 1, 10);

    if(ret == -1)
        log_tag_no(LOG_ERR, "poll");

    if(ret <= 0)
        return;

    if(fds.revents & POLLIN) {
        ret = recv(fd_, recvBuffer_, RECV_BUFFER_LEN, 0);

        if(ret == -1) {
            if(errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
                return;

            // broken connection
            status_ = Status::Failed;
            log_tag_no(LOG_ERR, "recv");

            if(errno == ETIMEDOUT || errno == ECONNRESET)
                reconnect();

            return;
        } else if(ret == 0) {
            // normal socket closure
            status_ = Status::Closed;
            return;
        }

        // actual data received here

        /*
          ret .. length for recv
          recvBuffer_ .. buffer for recv
          recvPacketBuffer_ .. buffer for packet
          recvPacketLength_ .. length for packet
          recvPacketPosition_ .. how filled is the packet already
        */

        int recvBufferIndex = 0;

        do {
            // read recvPacketPosition_
            while (recvPacketPosition_ < 4 && recvBufferIndex < ret) {
                recvPacketLength_ |= recvBuffer_[recvBufferIndex] << (3 - recvPacketPosition_) * 8;
                recvPacketPosition_++;
                recvBufferIndex++;
            }

            if (recvPacketPosition_ >= 4 && recvPacketBuffer_ == nullptr) {
                recvPacketBuffer_ = new uint8_t[recvPacketLength_];
            }

            size_t copyLength = std::min<size_t>(recvPacketLength_ - recvPacketPosition_, ret - recvBufferIndex);

            memcpy(recvPacketBuffer_ + recvPacketPosition_, recvBuffer_ + recvBufferIndex, copyLength);

            recvBufferIndex += copyLength;
            recvPacketPosition_ += copyLength;

            if (recvPacketPosition_ >= recvPacketLength_ - 1) {
                // do something with our packet here

                // delete packet
                delete recvPacketBuffer_;
                recvPacketLength_ = 0;
                recvPacketPosition_ = 0;
                recvPacketBuffer_ = nullptr;
            }
        } while (recvBufferIndex < ret);
    }

    if(fds.revents & POLLOUT) {
        if(status_ == Status::Connecting) {
            int error;
            socklen_t error_len = sizeof(error);

            if(getsockopt(fd_, SOL_SOCKET, SO_ERROR, &error, &error_len) == 0) {
                if(error == 0) { // successful connection
                    log_tag(LOG_INFO, "Connection successful");
                    status_ = Status::Connected;
                    return;
                } else { // connection failed
                    log_tag_no(LOG_ERR, "connection");
                    status_ = Status::Failed;
                    return;
                }
            } else {
                log_tag_no(LOG_ERR, "getsockopt");
                return;
            }
        }

        isWritable_ = true;
    }

    if(fds.revents & POLLERR || fds.revents & POLLHUP) {
        status_ = Status::Failed;
        int error;
        socklen_t error_len = sizeof(error);
        if(getsockopt(fd_, SOL_SOCKET, SO_ERROR, &error, &error_len) == 0) {
            if(error == 0)
                return;
            log_tag_no(LOG_ERR, "POLLERR");
            return;
        } else {
            log_tag_no(LOG_ERR, "getsockopt");
            return;
        }
    }
}

void TcpClient::close() {
    status_ = Status::Closed;
    if(fd_ >= 0) {
        ::shutdown(fd_, SHUT_RDWR);
        ::close(fd_);
    }
}

TcpClient::~TcpClient() {
    close();

    if(recvPacketBuffer_)
        delete[] recvPacketBuffer_;
}

#include <tcp_server.hh>
#include <logger.hh>

#include <string>
#include <cerrno>
#include <utility>
#include <sys/socket.h>
#include <sys/fcntl.h>
#include <arpa/inet.h>
#include <poll.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/poll.h>

TcpServer::TcpServer()
    : fd_(-1), client_(-1), sendPacketQueue_() { }

TcpServer::TcpServer(TcpServer&& other) {
    close();
    fd_ = std::exchange(other.fd_, -1);
    client_ = std::exchange(other.client_, -1);

    memcpy(buffer_, other.buffer_, BUFFER_LEN);
    recvPacket_ = std::move(other.recvPacket_);
    sendPacketQueue_ = std::move(other.sendPacketQueue_);
}

TcpServer& TcpServer::operator=(TcpServer&& other) {
    if(&other == this)
        return *this;

    close();

    fd_ = std::exchange(other.fd_, -1);
    client_ = std::exchange(other.client_, -1);
    memcpy(buffer_, other.buffer_, BUFFER_LEN);
    recvPacket_ = std::move(other.recvPacket_);
    sendPacketQueue_ = std::move(other.sendPacketQueue_);
    return *this;
}

bool TcpServer::open() {
    if((fd_ = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        log_tag_no(LOG_ERR, "open socket");
        return false;
    }

    if(fcntl(fd_, F_SETFL, O_NONBLOCK) == -1) {
        log_tag_no(LOG_ERR, "set O_NONBLOCK");
        close();
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
        log_tag_no(LOG_ERR, "bind");
        return false;
    }

    if(listen(fd_, 10) == -1) {
        log_tag_no(LOG_ERR, "listen");
        return false;
    }

    return true;
}

void TcpServer::handlePollServer(int revents) {
    if(revents & POLLIN) {
        sockaddr addr;
        socklen_t addr_len = sizeof(addr);

        int ret = accept(fd_, &addr, &addr_len);
        if(ret == -1) {
            if(errno == EAGAIN || errno == EWOULDBLOCK)
                return;

            log_tag_no(LOG_ERR, "accept");
            return;
        }

        if(client_ >= 0)
            closeClient();
        client_ = ret;
    }

    if(revents & POLLERR || revents & POLLHUP) {
        log_tag_no(LOG_ERR, "poll");
    }
}

bool TcpServer::handlePollinClient() {
    int ret;
    while(true) {
        ret = recv(client_, buffer_, BUFFER_LEN, 0);

        if(ret == -1) {
            if(errno != EAGAIN && errno != EWOULDBLOCK)
                return false;
            log_tag_no(LOG_ERR, "recv");
            return true;
        } else if(ret == 0) {
            closeClient();
            return true;
        }

        int buffer_consumed = 0;
        do {
            buffer_consumed += recvPacket_.write(std::span<uint8_t>(buffer_ + buffer_consumed, ret - buffer_consumed));
            if(recvPacket_.isPacketComplete()) {
                // TODO: do something with the packet
                recvPacket_ = {};
            }
        } while(buffer_consumed < ret);
    }
    return false;
}

bool TcpServer::handlePolloutClient() {
    int ret;
    do {
        TcpSendPacket& packet = sendPacketQueue_.front();
        size_t bytes_read = packet.read(buffer_, BUFFER_LEN);

        ret = send(client_, buffer_, bytes_read, 0);
        if(ret == -1) {
            if(errno == EAGAIN || errno == EWOULDBLOCK) {
                return false;
            }
            else {
                log_tag_no(LOG_ERR, "send");
                return true;
            }
        } else packet.advance(ret);

        if(packet.isPacketRead())
            sendPacketQueue_.pop();
    } while(!sendPacketQueue_.empty());
    return false;
}


void TcpServer::update() {
    int ret;
    pollfd fds[2] = {
        pollfd {
            .fd = fd_,
            .events = POLLIN,
        },
        pollfd {
            .fd = client_,
            .events = static_cast<short>(POLLIN | (sendPacketQueue_.empty() ? 0 : POLLOUT)),
            .revents = 0
        }
    };

    ret = poll(fds, client_ == -1 ? 1 : 2, 1000);

    if(ret == -1)
        log_tag_no(LOG_ERR, "poll");

    if(ret <= 0)
        return;

    handlePollServer(fds[0].revents);

    if(client_ >= 0) {
        const short& revents = fds[1].revents;
        if(revents & POLLIN) {
            if(handlePollinClient())
                return;
        }

        if(revents & POLLERR || revents & POLLHUP) {
            int error;
            socklen_t error_len = sizeof(error);

            if(getsockopt(client_, SOL_SOCKET, SO_ERROR, &error, &error_len) == 0) {
                if(error == 0)
                    return;
                log_tag_no(LOG_ERR, "POLLERR: " + std::string(strerror(error)));
            } else {
                log_tag_no(LOG_ERR, "getsockopt");
            }
            return;
        }

        if(revents & POLLOUT) {
            handlePolloutClient();
        }
    }
}

void TcpServer::closeClient() {
    if(client_ >= 0) {
        ::shutdown(client_, SHUT_RDWR);
        ::close(client_);
    }

    client_ = -1;
}

void TcpServer::close() {
    closeClient();

    if(fd_ >= 0) {
        ::close(fd_);
    }
}

TcpServer::~TcpServer() {
    close();
}

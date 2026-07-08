#include <socket_options.hh>
#include <tcp_client.hh>
#include <logger.hh>

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
    memcpy(buffer_, other.buffer_, BUFFER_LEN);
    packet_ = std::exchange(other.packet_, {});
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
    memcpy(buffer_, other.buffer_, BUFFER_LEN);
    packet_ = std::exchange(other.packet_, {});
    status_ = std::exchange(other.status_, Status::Failed);
    serverAddress_ = std::exchange(other.serverAddress_, {});
    reconnectCounter_ = std::exchange(other.reconnectCounter_, 0);
    isWritable_ = std::exchange(other.isWritable_, false);

    return *this;
}

bool TcpClient::open() {
    if((fd_ = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
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

bool TcpClient::handlePollin() {
    int ret;
    while(true) {
        ret = recv(fd_, buffer_, BUFFER_LEN, 0);

        if(ret == -1) {
            if(errno == EAGAIN || errno == EWOULDBLOCK) return false;

            // broken connection
            status_ = Status::Failed;
            log_tag_no(LOG_ERR, "recv");

            if(errno == ETIMEDOUT || errno == ECONNRESET)
                reconnect();
            return true;
        } else if(ret == 0) {
            // normal socket closure
            status_ = Status::Closed;
            return true;
        }

        // actual data received here
        int buffer_consumed = 0;
        do {
            buffer_consumed += packet_.write(std::span<uint8_t>(buffer_ + buffer_consumed, ret - buffer_consumed));

            if(packet_.isPacketComplete()) {
                // TODO: do something with the packet
                packet_ = {};
            }
        } while(buffer_consumed < ret);
    }
    return false;
}

bool TcpClient::handlePollout() {
    if(status_ == Status::Connecting) {
        int error;
        socklen_t error_len = sizeof(error);

        if(getsockopt(fd_, SOL_SOCKET, SO_ERROR, &error, &error_len) == 0) {
            if(error == 0) { // successful connection
                log_tag(LOG_INFO, "Connection successful");
                status_ = Status::Connected;
            } else { // connection failed
                log_tag_no(LOG_ERR, "connection");
                status_ = Status::Failed;
                return true;
            }
        } else {
            log_tag_no(LOG_ERR, "getsockopt");
            return true;
        }
    }

    isWritable_ = true;
    return false;
}

void TcpClient::handlePollerr() {
    status_ = Status::Failed;
    int error;
    socklen_t error_len = sizeof(error);
    if(getsockopt(fd_, SOL_SOCKET, SO_ERROR, &error, &error_len) == 0) {
        if(error == 0)
            return;
        log_tag_no(LOG_ERR, "POLLERR");
    } else {
        log_tag_no(LOG_ERR, "getsockopt");
    }
}

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
        if(handlePollin())
            return;
    }

    if((fds.revents & POLLERR) || (fds.revents & POLLHUP)) {
        handlePollerr();
        return;
    }

    if(fds.revents & POLLOUT) {
        if(handlePollout())
            return;
    } else if(checkWritable)
        isWritable_ = false;
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
}

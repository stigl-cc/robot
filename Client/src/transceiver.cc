#include <transceiver.hh>
#include <logger.hh>

#include <sys/poll.h>
#include <cerrno>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <asm-generic/socket.h>
#include <unistd.h>
#include <poll.h>

bool Transceiver::connect() {
    int ret = ::connect(fd_, reinterpret_cast<sockaddr*>(&serverAddress_), sizeof(serverAddress_));

    if(ret >= 0 || errno == EINPROGRESS) {
        status_ = Status::Connecting;
        return true;
    }

    log_tag_no("E", "connect");
    status_ = Status::Failed;
    return false;
}

bool Transceiver::reconnect() {
    if(reconnectCounter_++ < MAX_RECONNECTS) {
        close();
        return open();
    }
    return false;
}

Transceiver::Transceiver(sockaddr_in serverAddress)
    : fd_(-1), serverAddress_(serverAddress) {}

bool Transceiver::open() {
    if((fd_ = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        log_tag_no("E", "open socket");
        status_ = Status::Failed;
        return false;
    }

    if(fcntl(fd_, F_SETFL, O_NONBLOCK) < 0) {
        log_tag_no("E", "set O_NONBLOCK");
        close();
        status_ = Status::Failed;
        return false;
    }

    timeval timeout = { .tv_sec = TIMEOUT_S, .tv_usec = 0 };
    if(setsockopt(fd_, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        log_tag_no("W", "set SO_RCVTIMEO");
    }
    if(setsockopt(fd_, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0) {
        log_tag_no("W", "set SO_RCVTIMEO");
    }

    int reuse_addr = 0b1;
    if(setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(reuse_addr)) < 0) {
        log_tag_no("W", "set SO_REUSEADDR");
    }

    int keepalive = 0b1;
    if(setsockopt(fd_, SOL_SOCKET, SO_KEEPALIVE, &keepalive, sizeof(keepalive))) {
        log_tag_no("W", "set SO_KEEPALIVE");
    }

    sockaddr_in bind_address = {
        .sin_family = AF_INET,
        .sin_port = htons(BIND_PORT),
        .sin_addr = INADDR_ANY,
    };
    if(bind(fd_, reinterpret_cast<sockaddr*>(&bind_address), sizeof(bind_address)) < 0) {
        log_tag_no("W", "bind to address");
    }

    return connect();
}

Transceiver::Status Transceiver::getStatus() {
    return status_;
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

void Transceiver::update(bool checkWritable) {
    int ret;
    pollfd fds = {
        .fd = fd_,
        .events = POLLIN | ((status_ == Status::Connecting || checkWritable) ? POLLOUT : 0),
    };

    ret = poll(&fds, 1, 10);

    if(ret == -1)
        log_tag_no("E", "poll");

    if(ret <= 0)
        return;

    if(fds.revents & POLLIN) {
        ret = recv(fd_, recvBuffer_, RECV_BUFFER_LEN, 0);

        if(ret == -1) {
            if(errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
                return;

            // broken connection
            status_ = Status::Failed;
            log_tag_no("E", "recv");

            if(errno == ETIMEDOUT || errno == ECONNRESET)
                reconnect();
            return;
        } else if(ret == 0) {
            // normal socket closure
            status_ = Status::Closed;
            return;
        }

        // actual data received
    }

    if(fds.revents & POLLOUT) {
        if(status_ == Status::Connecting) {
            int error;
            socklen_t error_len = sizeof(error);

            if(getsockopt(fd_, SOL_SOCKET, SO_ERROR, &error, &error_len) == 0) {
                if(error == 0) { // successful connection
                    status_ = Status::Connected;
                    return;
                } else { // connection failed
                    log_tag_no("E", "connection");
                    status_ = Status::Failed;
                    return;
                }
            } else {
                log_tag_no("E", "getsockopt");
                return;
            }
        }
    }

    if(fds.revents & POLLERR || fds.revents & POLLHUP) {
        status_ = Status::Failed;
        int error;
        socklen_t error_len = sizeof(error);
        if(getsockopt(fd_, SOL_SOCKET, SO_ERROR, &error, &error_len) == 0) {
            if(error == 0)
                return;
            log_tag_no("E", "POLLERR");
            return;
        } else {
            log_tag_no("E", "getsockopt");
            return;
        }
    }
}

void Transceiver::close() {
    status_ = Status::Closed;
    ::shutdown(fd_, SHUT_RDWR);
    ::close(fd_);
}

Transceiver::~Transceiver() {
    close();
}

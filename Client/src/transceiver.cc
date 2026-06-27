#include <transceiver.hh>
#include <logger.hh>

#include <sys/poll.h>
#include <cerrno>
#include <sys/fcntl.h>
#include <cstring>
#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <asm-generic/socket.h>
#include <unistd.h>
#include <poll.h>

Transceiver::Transceiver() : fd(0) { }
Transceiver::~Transceiver() {
    this->Close();
}

bool Transceiver::Open(sockaddr_in server) {
    if((this->fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        log_tag_no("E", "open socket");
        return false;
    }

    if(fcntl(this->fd, F_SETFL, O_NONBLOCK) < 0) {
        log_tag_no("E", "set O_NONBLOCK");
        Close();
        return false;
    }

    timeval timeout = { .tv_sec = timeout_s, .tv_usec = 0 };
    if(setsockopt(this->fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        log_tag_no("W", "set SO_RCVTIMEO");
    }
    if(setsockopt(this->fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0) {
        log_tag_no("W", "set SO_RCVTIMEO");
    }

    int reuse_addr = 0b1;
    if(setsockopt(this->fd, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(reuse_addr)) < 0) {
        log_tag_no("W", "set SO_REUSEADDR");
    }

    int keepalive = 0b1;
    if(setsockopt(this->fd, SOL_SOCKET, SO_KEEPALIVE, &keepalive, sizeof(keepalive))) {
        log_tag_no("W", "set SO_KEEPALIVE");
    }

    sockaddr_in bind_address = {
        .sin_family = AF_INET,
        .sin_port = htons(bind_port),
        .sin_addr = INADDR_ANY,
    };
    if(bind(fd, reinterpret_cast<sockaddr*>(&bind_address), sizeof(bind_address)) < 0) {
        log_tag_no("W", "bind to address");
    }

    return this->Connect(server);
}

bool Transceiver::Connect(sockaddr_in server) {
    int ret = connect(fd, reinterpret_cast<sockaddr*>(&server), sizeof(server));

    if(ret >= 0 || errno == EAGAIN || errno == EWOULDBLOCK)
        return true;

    log_tag_no("E", "connect");
    return false;
}

void Transceiver::onDataAvilable() {
    int ret = recv(this->fd, buffer, recv_buffer_len, 0);
    if(ret >= 0 || errno == EAGAIN || errno == EWOULDBLOCK)
        return;
    log_tag_no("W", "recv");
}

bool Transceiver::isWritable() {
    pollfd fds = {
        .fd = this->fd,
        .events = POLLIN | POLLERR | POLLHUP | POLLOUT,
    };

    return this->handlePoll(fds) & POLLOUT;
}

short Transceiver::handlePoll(pollfd fds) {
    int ret = poll(&fds, 1, 10);

    if(ret == -1) {
        log_tag_no("E", "poll");
        return -1;
    } else if(ret == 0) {
        return -1; // nothing
    } else {
        if(fds.revents & POLLERR || fds.revents & POLLHUP) {
            int error;
            socklen_t error_len;
            getsockopt(this->fd, SOL_SOCKET, SO_ERROR, &error, &error_len);
        }

        if(fds.revents & POLLIN)
            onDataAvilable();
        return fds.revents;
    }
}

void Transceiver::UpdateEvents() {
    pollfd fds = {
        .fd = this->fd,
        .events = POLLIN | POLLHUP | POLLERR,
    };

    int ret = poll(&fds, 1, 10);
    if(ret == -1) {
        log_tag_no("E", "poll socket events");
    } else if(ret == 0) {

    } else {
        if(fds.revents & POLLIN)
            onDataAvilable();
    }
}

void Transceiver::Close() {
    close(fd);
}

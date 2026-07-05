#include <tcp_server.hh>
#include <logger.hh>

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
    : fd_(-1) { }

TcpServer::TcpServer(TcpServer&& other) {
    fd_ = std::exchange(other.fd_, -1);
}

TcpServer& TcpServer::operator=(TcpServer&& other) {
    if(&other == this)
        return *this;

    fd_ = std::exchange(other.fd_, -1);
    return *this;
}

bool TcpServer::open() {
    if((fd_ = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        log_tag_no(LOG_ERR, "open socket");
        return false;
    }

    if(fcntl(fd_, F_SETFL, O_NONBLOCK) < 0) {
        log_tag_no(LOG_ERR, "set O_NONBLOCK");
        close();
        return false;
    }

    if(!SocketOptions::setTimeout(fd_, TIMEOUT_SEC * 1'000)) {
        log_tag_no(LOG_WARN, "set Timeout");
    }

    int reuse_addr = 0b1;
    if(setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(reuse_addr)) < 0) {
        log_tag_no(LOG_WARN, "set SO_REUSEADDR");
    }

    if(!SocketOptions::setKeepAlive(fd_, KA_OPTIONS)) {
        log_tag(LOG_WARN, "set up KeepAlive");
    }

    sockaddr_in bind_address = {
        .sin_family = AF_INET,
        .sin_port = htons(BIND_PORT),
        .sin_addr = INADDR_ANY,
    };

    if(bind(fd_, reinterpret_cast<sockaddr*>(&bind_address), sizeof(bind_address)) < 0) {
        log_tag_no(LOG_ERR, "bind to address");
        return false;
    }

    return true;
}

void TcpServer::close() {
    if(fd_ >= 0) {
        
    }
}

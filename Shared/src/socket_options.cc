#include <socket_options.hh>
#include <logger.hh>

#include <sys/socket.h>
#include <netinet/tcp.h>

bool SocketOptions::setTimeout(int fd, uint32_t msTimeout) {
    timeval timeout = { .tv_sec = msTimeout / 1'000, .tv_usec = msTimeout % 1'000 };
    if(setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        log_tag_no(LOG_WARN, "set SO_RCVTIMEO");
        return false;
    }

    if(setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0) {
        log_tag_no(LOG_WARN, "set SO_RCVTIMEO");
        return false;
    }
    return true;
}

bool SocketOptions::setKeepAlive(int fd, KeepAliveOptions options) {
    if(setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &options.enabled, sizeof(options.enabled))) {
        log_tag_no(LOG_WARN, "set SO_KEEPALIVE");
        return false;
    }

    if(setsockopt(fd, SOL_TCP, TCP_KEEPCNT, &options.probes, sizeof(options.probes)) < 0) {
        log_tag_no(LOG_WARN, "set SO_KEEPCNT");
        return false;
    }

    if(setsockopt(fd, SOL_TCP, TCP_KEEPIDLE, &options.idleTime, sizeof(options.idleTime) ) < 0) {
        log_tag_no(LOG_WARN, "set SO_KEEPIDLE");
        return false;
    }

    if(setsockopt(fd, SOL_TCP, TCP_KEEPINTVL, &options.interval, sizeof(options.interval)) < 0) {
        log_tag_no(LOG_WARN, "set SO_KEEPINTVL");
        return false;
    }

    return true;
}

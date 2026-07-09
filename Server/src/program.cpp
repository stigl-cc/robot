#include <tcp_server.hh>
#include <logger.hh>

#include <csignal>
#include <atomic>

std::atomic<bool> shouldApplicationRun = true;

void signal_handler(int) {
    shouldApplicationRun = false;
}

int main() {
    shouldApplicationRun = true;

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    TcpServer tcpServer;

    log(LOG_INFO, "Starting TCP listener");
    tcpServer.open();

    while(shouldApplicationRun) {
        tcpServer.update();
    }

    log(LOG_INFO, "Closing TCP listener");
    tcpServer.close();

    log(LOG_INFO, "Exiting application");
    return 0;
}

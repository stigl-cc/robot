#include <language_model.hh>
#include <tcp_server.hh>
#include <logger.hh>

#include <csignal>
#include <atomic>

std::atomic<bool> should_application_run = true;

void signal_handler(int) {
    should_application_run = false;
}

int main() {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    TcpServer tcp_server;
    LanguageModel language_model;
    language_model.test();

    log(LOG_INFO, "Starting TCP listener");
    tcp_server.open();

    while(should_application_run) {
        tcp_server.update();
    }

    log(LOG_INFO, "Closing TCP listener");
    tcp_server.close();

    log(LOG_INFO, "Exiting server");
    return 0;
}

#include "packet.hh"
#include <iterator>
#include <language_model.hh>
#include <tcp_server.hh>
#include <logger.hh>

#include <csignal>
#include <atomic>
#include <type_traits>

std::atomic<bool> should_application_run = true;

void signal_handler(int) {
    should_application_run = false;
}

template<typename RandomAccesIterator_> bool check_jpeg_header(RandomAccesIterator_ begin, RandomAccesIterator_ end) {
    static_assert(std::is_same<uint8_t, std::iter_value_t<RandomAccesIterator_>>::value, "Iterator type not a unit8_t random access iterator!");

    if(std::distance(begin, end) < 4)
        return false;

    return
        static_cast<uint8_t>(*(begin    )) == 0xFF &&
        static_cast<uint8_t>(*(begin + 1)) == 0xD8 &&
        static_cast<uint8_t>(*(end   - 2)) == 0xFF &&
        static_cast<uint8_t>(*(end   - 1)) == 0xD9;
}

void tcp_recv_task(LanguageModel& language_model, const TcpRecvPacket& packet) {
    const std::vector<uint8_t>& buffer = packet.getBuffer();
    // Check if buffer type is a JPEG
    if(check_jpeg_header(buffer.begin(), buffer.end())) {
        std::cout << "Loaded JPEG Image\n";
        language_model.load_media(packet.getBuffer());
    }
}

int main() {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    TcpServer tcp_server;
    LanguageModel language_model;
    language_model.open();

    tcp_server.getRecvEvent().subscribe(std::bind(tcp_recv_task, std::ref(language_model), std::placeholders::_1));

    log(LOG_INFO, "Starting TCP listener");
    tcp_server.open();

    while(should_application_run) {
        tcp_server.update();
    }

    language_model.load_text("Describe the contents of the image(s): ");
    language_model.tokenize_inputs();
    std::cout << language_model.generate_text() << "\n";

    log(LOG_INFO, "Closing TCP listener");
    tcp_server.close();

    log(LOG_INFO, "Exiting server");
    return 0;
}

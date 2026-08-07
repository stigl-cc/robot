#include <functional>
#include <packet.hh>
#include <scheduler.hh>
#include <logger.hh>
#include <tcp_client.hh>
#include <video_capture.hh>
#include <servo_controller.hh>
#include <gyroscope.hh>

#include <span>
#include <csignal>
#include <atomic>
#include <arpa/inet.h>

TcpClient::Status last_status = TcpClient::Status::Closed;
std::atomic<bool> should_application_run = true;

void signal_handler(int) {
    should_application_run = false;
}

void video_capture_task(VideoCapture& videoCapture, TcpClient& tcpClient) {
    videoCapture.captureFrame();
    const uint8_t* video_buffer;
    size_t video_buffer_len = videoCapture.getVideoBuffer(reinterpret_cast<const void**>(&video_buffer));

    std::span<const uint8_t> video_buffer_span {video_buffer, video_buffer_len};
    tcpClient.send(TcpSendPacket(video_buffer_span));
}

void tcp_recv_task(TcpClient& client, const TcpRecvPacket& packet) {

}

void gyro_update_task(Gyroscope& gyro) {
    gyro.update();
}

void tcp_update_task(TcpClient& tcpClient) {
    tcpClient.update();

    TcpClient::Status status = tcpClient.getStatus();
    if(last_status != status) {
        switch(status) {
            case TcpClient::Status::Connecting:
                std::cout << "Connecting\n";
                break;
            case TcpClient::Status::Connected:
                std::cout << "Connected\n";
                break;
            case TcpClient::Status::Closed:
                std::cout << "Closed\n";
                break;
            case TcpClient::Status::Failed:
                std::cout << "Failed\n";
                break;
        }
    }
    last_status = status;
}

int main() {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    TaskScheduler task_scheduler;

    Gyroscope gyroscope(1);
    log(LOG_INFO, "Initializing gyroscope");
    std::cout << "Do not move the Gyro, calibrating!\n";
    gyroscope.calibrate();

    ServoController servo_controller(1);
    log(LOG_INFO, "Initializing servo controller");
    servo_controller.open();
    servo_controller.setOscillatorFrequency(27'000'000);
    servo_controller.setPWMFreq(50.0f);

    VideoCapture video_capture(0);
    log(LOG_INFO, "Initializing video capture");
    video_capture.open();

    TcpClient tcp_client("192.168.0.36", 8080);
    log(LOG_INFO, "Starting TCP client");

    tcp_client.getRecvEvent().subscribe(std::bind(tcp_recv_task, std::ref(tcp_client), std::placeholders::_1));
    tcp_client.open();

    task_scheduler.registerTask({ std::bind(gyro_update_task, std::ref(gyroscope)), Task::timeunit_t(100), true });
    task_scheduler.registerTask({ std::bind(video_capture_task, std::ref(video_capture), std::ref(tcp_client)), Task::timeunit_t(10'000), true });
    task_scheduler.registerTask({ std::bind(tcp_update_task, std::ref(tcp_client)), Task::timeunit_t(100), true });

    task_scheduler.start();

    while(should_application_run) {
        usleep(10'000);
    }

    log(LOG_INFO, "Stopping task scheduler");
    task_scheduler.stop();

    log(LOG_INFO, "Closing TCP client");
    tcp_client.close();

    log(LOG_INFO, "Closing video capture");
    video_capture.close();

    log(LOG_INFO, "Exiting Client");
    return 0;
}

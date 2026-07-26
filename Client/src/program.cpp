#include <logger.hh>
#include <tcp_client.hh>
#include <video_capture.hh>
#include <servo_controller.hh>
#include <gyroscope.hh>

#include <csignal>
#include <atomic>
#include <arpa/inet.h>

TcpClient::Status last_status = TcpClient::Status::Closed;
std::atomic<bool> should_application_run = true;

void signal_handler(int) {
    should_application_run = false;
}

int main() {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    sockaddr_in listen_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(8080)
    };

    inet_pton(AF_INET, "127.0.0.1", &listen_addr.sin_addr);

    TcpClient tcp_client(listen_addr);

    log(LOG_INFO, "Starting TCP client");
    tcp_client.open();

    uint8_t data[1] = { 0x00 };
    TcpSendPacket packet_to_send(std::span<uint8_t>(reinterpret_cast<uint8_t*>(data), 1));
    tcp_client.send(packet_to_send);

    while(should_application_run) {
        TcpClient::Status status = tcp_client.getStatus();
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

        tcp_client.update();
        usleep(100'000);
    }

    log(LOG_INFO, "Closing TCP client");
    tcp_client.close();

    log(LOG_INFO, "Exiting Client");
    return 0;
}

/*
  uint8_t servo_min = 0, servo_max = 5;
ServoController servo(1);

std::cout << "Initializing servo with " << servo.Begin(false) << "\n";
std::cout << (int)servo_min << " < servo < " << (int)servo_max << std::endl;

servo.SetOscillatorFrequency(27'000'000);
servo.SetPWMFreq(50.0f);

usleep(10'000);

for(uint16_t pulse = 160; pulse < 600; pulse++) {
    for(uint8_t channel = servo_min; channel < servo_max; channel++) {
        servo.SetPWM(channel, 0, pulse);
    }
    usleep(10'000);
    }*/

/*
Gyroscope gyro(1);
std::cout << "Initializing gyro with " << gyro.Begin() << "\n";
std::cout << "Do not move the Gyro, calibrating!\n";
gyro.Calibrate();
while(true) {
    gyro.Update();

    std::cout << "Acceleration: " << std::setprecision(5) << gyro.accel.X << ", " << gyro.accel.Y << ", " << gyro.accel.Z << "\n"
              << "Gyroscope: " << std::setprecision(5) << gyro.gyro.X << ", " << gyro.gyro.Y << ", " << gyro.gyro.Z << "\n"
              << "Angle: " << std::setprecision(5) << gyro.angle.X << ", " << gyro.angle.Y << ", " << gyro.angle.Z << "\n"
              << "Temp: " << std::setprecision(5) << gyro.temp << "\n\n\n";
    usleep(300'000);
}

*/

/*
  VideoCapture video(0);

const void* video_buffer = nullptr;
int video_buffer_len;
video_buffer_len = video.GetVideoBuffer(&video_buffer);

video.CaptureFrame();

std::cerr << video.Open() << "\n";
*/

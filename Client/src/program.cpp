#include "video_capture.hh"
#include <iostream>
#include <servo_controller.hh>
#include <gyroscope.hh>
#include <unistd.h>
#include <linux/videodev2.h>

int main() {
    /*    uint8_t servo_min = 0, servo_max = 5;
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
    VideoCapture video(0);

    const void* video_buffer = nullptr;
    int video_buffer_len;
    video_buffer_len = video.GetVideoBuffer(&video_buffer);

    video.CaptureFrame();

    std::cerr << video.Open() << "\n";
}

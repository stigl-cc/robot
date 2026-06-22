#include <servo_controller.hh>
#include <unistd.h>
#include <iostream>
int main() {
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
    }
}

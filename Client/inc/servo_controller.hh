#pragma once
#include <cstdint>
#include <i2c_device.hh>

// PCA9685
class ServoController : private I2cDevice {
    protected:
    static constexpr std::string_view LOG_TAG = "ServoController: ";
    static constexpr uint8_t
        MAX_SERVOS = 16,
        I2C_ADDRESS = 0x40;

    uint32_t oscillator_freq_;

    public:
    ServoController(int i2cAdapter);

    bool open(uint8_t prescale);
    void reset();
    void sleep();
    void wakeUp();

    void setExtClock(uint8_t prescale);
    void setPWMFreq(float freq);
    void setOutputMode(bool totempole);

    uint16_t getPWM(uint8_t channel, bool off);
    void setPWM(uint8_t channel, uint16_t on, uint16_t off);
    void setPin(uint8_t channel, uint16_t val, bool invert);

    uint8_t readPrescale();

    void setOscillatorFrequency(uint32_t freq);
    uint32_t getOscillatorFrequency();
    void write_us(uint8_t channel, uint16_t uS);
};

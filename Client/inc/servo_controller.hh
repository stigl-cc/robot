#pragma once
#include <cstdint>
#include <i2c_device.hh>

// PCA9685
class ServoController : private I2cDevice {
    protected:
    static constexpr uint8_t
        max_servos = 16,
        i2c_address = 0x40;

    uint32_t oscillator_freq;

    public:
    ServoController(int i2c_adapter);

    bool Begin(uint8_t prescale);
    void Reset();
    void Sleep();
    void WakeUp();

    void SetExtClock(uint8_t prescale);
    void SetPWMFreq(float freq);
    void SetOutputMode(bool totempole);

    uint16_t GetPWM(uint8_t channel, bool off);
    void SetPWM(uint8_t channel, uint16_t on, uint16_t off);
    void SetPin(uint8_t channel, uint16_t val, bool invert);

    uint8_t ReadPrescale();

    void SetOscillatorFrequency(uint32_t freq);
    uint32_t GetOscillatorFrequency();
    void Write_us(uint8_t channel, uint16_t uS);
};

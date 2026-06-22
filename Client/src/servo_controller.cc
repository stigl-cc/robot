#include <i2c_device.hh>
#include <servo_controller.hh>

#include <cmath>
#include <cstdint>
#include <unistd.h>

// REGISTER ADDRESSES
#define PCA9685_MODE1 0x00
#define PCA9685_MODE2 0x01
#define PCA9685_SUBADR1 0x02
#define PCA9685_SUBADR2 0x03
#define PCA9685_SUBADR3 0x04
#define PCA9685_ALLCALLADR 0x05
#define PCA9685_LED0_ON_L 0x06
#define PCA9685_LED0_ON_H 0x07
#define PCA9685_LED0_OFF_L 0x08
#define PCA9685_LED0_OFF_H 0x09
// etc all 16:  LED15_OFF_H 0x45
#define PCA9685_ALLLED_ON_L 0xFA
#define PCA9685_ALLLED_ON_H 0xFB
#define PCA9685_ALLLED_OFF_L 0xFC
#define PCA9685_ALLLED_OFF_H 0xFD
#define PCA9685_PRESCALE 0xFE
#define PCA9685_TESTMODE 0xFF

// MODE1 bits
#define MODE1_ALLCAL 0x01
#define MODE1_SUB3 0x02
#define MODE1_SUB2 0x04
#define MODE1_SUB1 0x08
#define MODE1_SLEEP 0x10
#define MODE1_AI 0x20
#define MODE1_EXTCLK 0x40
#define MODE1_RESTART 0x80
// MODE2 bits
#define MODE2_OUTNE_0 0x01
#define MODE2_OUTNE_1 0x02
#define MODE2_OUTDRV 0x04
#define MODE2_OCH 0x08
#define MODE2_INVRT 0x10

#define FREQUENCY_OSCILLATOR 25000000

#define PCA9685_PRESCALE_MIN 3
#define PCA9685_PRESCALE_MAX 255

ServoController::ServoController(int i2c_adapter) : I2cDevice(i2c_adapter, i2c_address) {
}

bool ServoController::Begin(uint8_t prescale) {
    if(!((I2cDevice*)this)->Open()) {
        return false;
    }
    this->Reset();
    this->SetOscillatorFrequency(FREQUENCY_OSCILLATOR);

    if(prescale) this->SetExtClock(prescale);
    else this->SetPWMFreq(1000.0f);
    return true;
}

void ServoController::Reset() {
    WriteReg(PCA9685_MODE1, MODE1_RESTART);

    usleep(10'000);
}

void ServoController::Sleep() {
    uint8_t buffer[2] = { PCA9685_MODE1, 0 };
    WriteRead(buffer, 1, buffer + 1, 1);

    buffer[1] |= MODE1_SLEEP;
    Write(buffer, 2);
    usleep(5'000);
}

void ServoController::WakeUp() {
    uint8_t buffer[2] = { PCA9685_MODE1, 0 };
    WriteRead(buffer, 1, buffer + 1, 1);

    buffer[1] &= ~MODE1_SLEEP;
    Write(buffer, 2);
}

void ServoController::SetExtClock(uint8_t prescale) {
    uint8_t mode = (WriteReadReg(PCA9685_MODE1) & ~MODE1_RESTART) | MODE1_SLEEP;

    WriteReg(PCA9685_MODE1, mode);
    mode |= MODE1_EXTCLK;
    WriteReg(PCA9685_MODE1, mode);

    WriteReg(PCA9685_PRESCALE, prescale);
    usleep(5'000);

    WriteReg(PCA9685_MODE1, (mode & ~MODE1_SLEEP) | MODE1_RESTART | MODE1_AI);
}

void ServoController::SetPWMFreq(float freq) {
    //uint8_t buffer[2] = { PCA9685_MODE1, 0 };
    freq = std::fmaxf(std::fminf(freq, 3500), 1);

    float prescaleval = ((oscillator_freq / (freq * 4096.0)) + 0.5) - 1;
    prescaleval = std::fmaxf(std::fminf(prescaleval, PCA9685_PRESCALE_MAX), PCA9685_PRESCALE_MIN);

    uint8_t oldmode = WriteReadReg(PCA9685_MODE1);
    uint8_t newmode = (oldmode & ~MODE1_RESTART) | MODE1_SLEEP;

    WriteReg(PCA9685_MODE1, newmode);
    WriteReg(PCA9685_PRESCALE, uint8_t(prescaleval));
    WriteReg(PCA9685_MODE1, oldmode);

    usleep(5'000);

    WriteReg(PCA9685_MODE1, oldmode | MODE1_RESTART | MODE1_AI);
}

void ServoController::SetOutputMode(bool totempole) {
    uint8_t mode = WriteReadReg(PCA9685_MODE2);

    mode = totempole
        ? mode | MODE2_OUTDRV
        : mode &~MODE2_OUTDRV;

    WriteReg(PCA9685_MODE2, mode);
}

uint16_t ServoController::GetPWM(uint8_t channel, bool off) {
    uint8_t buffer[2] = { uint8_t(PCA9685_LED0_ON_L + 4 * channel + (off ? 2 : 0)), 0 };
    WriteRead(buffer, 1, buffer, 2);

    return (uint16_t(buffer[1]) << 8) | buffer[0];
}

void ServoController::SetPWM(uint8_t channel, uint16_t on, uint16_t off) {
    uint8_t buffer[5] = {
        uint8_t(PCA9685_LED0_ON_L + 4 * channel),
        uint8_t(on),
        uint8_t(on >> 8),
        uint8_t(off),
        uint8_t(off >> 8)
    };

    Write(buffer, 5);
}

void ServoController::SetPin(uint8_t channel, uint16_t val, bool invert) {
    val = std::min(val, (uint16_t)4095);

    if(invert)
        val = 4095 - val;

    if(val == 0)
        this->SetPWM(channel, 0, 4096);
    else if(val == 4095)
        this->SetPWM(channel, 4096, 0);
    else
        this->SetPWM(channel, 0, val);
}

uint8_t ServoController::ReadPrescale() {
    return WriteReadReg(PCA9685_PRESCALE);
}

void ServoController::Write_us(uint8_t channel, uint16_t us) {
    double us_per_bit = 1'000'000.0 * (this->ReadPrescale() + 1.0) / oscillator_freq;
    this->SetPWM(channel, 0, uint16_t(us / us_per_bit));
}

void ServoController::SetOscillatorFrequency(uint32_t freq) {
    oscillator_freq = freq;
}

uint32_t ServoController::GetOscillatorFrequency() {
    return oscillator_freq;
}

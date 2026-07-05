#pragma once
#include <cstdint>
#include <string_view>

class I2cDevice {
    static constexpr std::string_view LOG_TAG = "I2C";
    protected:
    int fd_;
    int adapter_;
    uint8_t address_;

    public:
    I2cDevice(int adapter, uint8_t address);

    I2cDevice(const I2cDevice&) = delete;
    I2cDevice& operator=(const I2cDevice&) = delete;

    I2cDevice(I2cDevice&&);
    I2cDevice& operator=(I2cDevice&&);

    bool open();

    bool write(uint8_t* buffer, uint16_t bufferLength);
    bool writeReg(uint8_t reg, uint8_t data);

    void read(uint8_t* buffer, uint16_t bufferLength);

    void writeRead(uint8_t* wBuffer, uint16_t wLen, uint8_t* rBuffer, uint16_t rLen);
    uint8_t writeReadReg(uint8_t reg);

    void close();

    ~I2cDevice();
};

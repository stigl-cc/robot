#pragma once
#include <cstdint>

class I2cDevice {
    protected:
    int fd;
    int adapter;
    uint8_t address;

    public:
    I2cDevice(const I2cDevice&) = delete;
    I2cDevice operator=(const I2cDevice&) = delete;

    I2cDevice(I2cDevice&&);
    I2cDevice& operator=(I2cDevice&&);
    ~I2cDevice();

    I2cDevice(int adapter, uint8_t address);
    bool Open();
    void Close();

    uint8_t WriteReadReg(uint8_t reg);
    void WriteReg(uint8_t reg, uint8_t data);

    void Write(uint8_t* buffer, uint16_t buffer_length);
    void Read(uint8_t* buffer, uint16_t buffer_length);
    void WriteRead(uint8_t* w_buffer, uint16_t w_len, uint8_t* r_buffer, uint16_t r_len);
};

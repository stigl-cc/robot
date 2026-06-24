#include <i2c_device.hh>

#include <format>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <fcntl.h>
#include <string>
#include <sys/ioctl.h>
#include <unistd.h>
#include <utility>

I2cDevice::I2cDevice(I2cDevice&& other) {
    fd = std::exchange(other.fd, 0);
    adapter = other.adapter;
    address = other.address;
}

I2cDevice& I2cDevice::operator=(I2cDevice&& other) {
    if(this == &other)
        return *this;

    fd = std::exchange(other.fd, 0);
    adapter = other.adapter;
    address = other.address;
    return *this;
}

I2cDevice::I2cDevice(int adapter, uint8_t address)
    : fd(0),
      adapter(adapter),
      address(address)
       { }

I2cDevice::~I2cDevice() {
    Close();
}

bool I2cDevice::Open() {
    if(fd > 0)
        return false;

    std::string device_file = std::format("/dev/i2c-{}", adapter);
    if((fd = open(device_file.c_str(), O_RDWR)) < 0) { return false; }

    if(ioctl(fd, I2C_SLAVE, address) < 0) {
        Close();
        return false;
    }

    return true;
}

void I2cDevice::Close() {
    if(fd) {
        close(fd);
        fd = 0;
    }
}

void I2cDevice::WriteReg(uint8_t reg, uint8_t data) {
    uint8_t buffer[2] = { reg, data };
    Write(buffer, 2);
}

uint8_t I2cDevice::WriteReadReg(uint8_t reg) {
    uint8_t buffer = reg;
    WriteRead(&buffer, 1, &buffer, 1);
    return buffer;
}

void I2cDevice::Write(uint8_t* buffer, uint16_t buffer_length) {
    i2c_msg msg = {
        .addr = address,
        .flags = 0, // write
        .len = buffer_length,
        .buf = buffer,
    };

    i2c_rdwr_ioctl_data ioctl_data = {
        .msgs = &msg,
        .nmsgs = 1,
    };

    ioctl(fd, I2C_RDWR, &ioctl_data);
}

void I2cDevice::Read(uint8_t* buffer, uint16_t length) {
    i2c_msg msg = {
        .addr = address,
        .flags = I2C_M_RD,
        .len = length,
        .buf = buffer,
    };

    i2c_rdwr_ioctl_data ioctl_data = {
        .msgs = &msg,
        .nmsgs = 1,
    };

    ioctl(fd, I2C_RDWR, &ioctl_data);
}

void I2cDevice::WriteRead(uint8_t* w_buffer, uint16_t w_len, uint8_t* r_buffer, uint16_t r_len) {
    i2c_msg msgs[2] = {
        {
            .addr = address,
            .flags = 0,
            .len = w_len,
            .buf = w_buffer,
        },
                {
            .addr = address,
            .flags = I2C_M_RD,
            .len = r_len,
            .buf = r_buffer,
        }
    };

    i2c_rdwr_ioctl_data ioctl_data = {
        .msgs = msgs,
        .nmsgs = 2,
    };

    ioctl(fd, I2C_RDWR, &ioctl_data);
}

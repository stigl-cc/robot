#include <i2c_device.hh>

#include <format>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <fcntl.h>
#include <string>
#include <sys/ioctl.h>
#include <unistd.h>
#include <utility>


I2cDevice::I2cDevice(int adapter, uint8_t address)
    : fd_(-1),
      adapter_(adapter),
      address_(address)
       { }

I2cDevice::I2cDevice(I2cDevice&& other) : fd_(-1) {
    fd_ = std::exchange(other.fd_, -1);
    adapter_ = other.adapter_;
    address_ = other.address_;
}

I2cDevice& I2cDevice::operator=(I2cDevice&& other) {
    if(this == &other)
        return *this;

    if(fd_ >= 0) close();
    fd_ = std::exchange(other.fd_, -1);
    adapter_ = std::exchange(other.adapter_, 0);
    address_ = std::exchange(other.address_, 0);
    return *this;
}

bool I2cDevice::open() {
    if(fd_ >= 0)
        return false;

    std::string device_file = std::format("/dev/i2c-{}", adapter_);
    if((fd_ = ::open(device_file.c_str(), O_RDWR)) < 0) { return false; }

    if(ioctl(fd_, I2C_SLAVE, address_) < 0) {
        close();
        return false;
    }

    return true;
}

bool I2cDevice::write(uint8_t* buffer, uint16_t bufferLength) {
    i2c_msg msg = {
        .addr = address_,
        .flags = 0, // write
        .len = bufferLength,
        .buf = buffer,
    };

    i2c_rdwr_ioctl_data ioctl_data = {
        .msgs = &msg,
        .nmsgs = 1,
    };

    return ioctl(fd_, I2C_RDWR, &ioctl_data) >= 0;
}

bool I2cDevice::writeReg(uint8_t reg, uint8_t data) {
    uint8_t buffer[2] = { reg, data };
    return write(buffer, 2);
}

void I2cDevice::read(uint8_t* buffer, uint16_t bufferLength) {
    i2c_msg msg = {
        .addr = address_,
        .flags = I2C_M_RD,
        .len = bufferLength,
        .buf = buffer,
    };

    i2c_rdwr_ioctl_data ioctl_data = {
        .msgs = &msg,
        .nmsgs = 1,
    };

    ioctl(fd_, I2C_RDWR, &ioctl_data);
}

void I2cDevice::writeRead(uint8_t* wBuffer, uint16_t wLen, uint8_t* rBuffer, uint16_t rLen) {
    i2c_msg msgs[2] = {
        {
            .addr = address_,
            .flags = 0,
            .len = wLen,
            .buf = wBuffer,
        },
                {
            .addr = address_,
            .flags = I2C_M_RD,
            .len = rLen,
            .buf = rBuffer,
        }
    };

    i2c_rdwr_ioctl_data ioctl_data = {
        .msgs = msgs,
        .nmsgs = 2,
    };

    ioctl(fd_, I2C_RDWR, &ioctl_data);
}

uint8_t I2cDevice::writeReadReg(uint8_t reg) {
    uint8_t buffer = reg;
    writeRead(&buffer, 1, &buffer, 1);
    return buffer;
}

void I2cDevice::close() {
    if(fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
}

I2cDevice::~I2cDevice() {
    close();
}

#pragma once
#include <i2c_device.hh>
#include <vectors.hh>

#include <chrono>
#include <cstdint>

// MPU6050
class Gyroscope : private I2cDevice {
    protected:
    const char* LOG_TAG = "Gyroscope: ";
    static constexpr uint8_t
        I2C_ADDRESS = 0x68;

    bool is_upside_down_ = true, angle_unset_ = true;
    float gyro_lsb_to_degsec_, accel_lsb_to_g_;

    Vector3 offset_accel_, offset_gyro_;
    std::chrono::steady_clock::time_point last_time_;

    public:
    Vector3 angle;
    float temp;
    Vector3 accel, gyro;

    Gyroscope(int i2cAdapter);

    bool open();
    void calibrate();

    void calculateAngles(double deltaTime);
    void readRawData();
    void update();

    uint8_t
        setGyroConfig(int),
        setAccelConfig(int);
};

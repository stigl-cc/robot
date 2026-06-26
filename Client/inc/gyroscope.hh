#pragma once
#include <i2c_device.hh>
#include <vectors.hh>

#include <chrono>
#include <cstdint>

// MPU6050
class Gyroscope : private I2cDevice {
    protected:
    const char* log_tag = "Gyroscope: ";
    static constexpr uint8_t
        i2c_address = 0x68;

    bool is_upside_down = true, angle_unset = true;
    float gyro_lsb_to_degsec, accel_lsb_to_g;

    Vector3 offset_accel, offset_gyro;

    std::chrono::steady_clock::time_point last_time;
    public:
    Vector3 angle;
    float temp;
    Vector3 accel, gyro;

    Gyroscope(int i2c_adapter);

    bool Open();
    void Calibrate();

    void CalculateAngles(double deltaTime);
    void ReadRawData();
    void Update();

    uint8_t
        SetGyroConfig(int),
        SetAccelConfig(int);
};

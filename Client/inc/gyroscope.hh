#pragma once
#include <i2c_device.hh>
#include <vectors.hh>

#include <cstdint>

// MPU6050
class Gyroscope : private I2cDevice {
    protected:
    static constexpr uint8_t
        i2c_address = 0x68;

    public:
    Gyroscope(int i2c_adapter);

    bool Begin();

    void CalcGyroOffsets(), CalcAccelOffsets();

    uint8_t SetGyroConfig(int), SetAccelConfig(int);

    void SetFilterGyroCoef(float), SetFilterAccelCoef(float);
    
    Vector3 SetGyroOffset(), SetAccelOffset();
    Vector3 GetGyroOffset(), GetAccelOffset();

    Vector3 GetAccel();
    Vector3 GetAccelAngle();

    Vector3 GetAngle();
};

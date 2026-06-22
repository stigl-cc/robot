#include <gyroscope.hh>
#include <i2c_device.hh>

#define MPU6050_ADDR                  0x68
#define MPU6050_SMPLRT_DIV_REGISTER   0x19
#define MPU6050_CONFIG_REGISTER       0x1a
#define MPU6050_GYRO_CONFIG_REGISTER  0x1b
#define MPU6050_ACCEL_CONFIG_REGISTER 0x1c
#define MPU6050_PWR_MGMT_1_REGISTER   0x6b

#define MPU6050_GYRO_OUT_REGISTER     0x43
#define MPU6050_ACCEL_OUT_REGISTER    0x3B

#define RAD_2_DEG             57.29578 // [deg/rad]
#define CALIB_OFFSET_NB_MES   500
#define TEMP_LSB_2_DEGREE     340.0    // [bit/celsius]
#define TEMP_LSB_OFFSET       12412.0

#define DEFAULT_GYRO_COEFF    0.98

Gyroscope::Gyroscope(int i2c_adapter) : I2cDevice(i2c_adapter, i2c_address) {}

bool Gyroscope::Begin() {
    if(!((I2cDevice*)this)->Open())
        return false;
    
    uint8_t buffer[2] = { MPU6050_PWR_MGMT_1_REGISTER, 0x01 };
    
    return true;
}

void Gyroscope::CalcGyroOffsets() {
    
}

void Gyroscope::CalcAccelOffsets();

uint8_t Gyroscope::SetGyroConfig(int);
uint8_t Gyroscope::SetAccelConfig(int);

void Gyroscope::SetFilterGyroCoef(float);
void Gyroscope::SetFilterAccelCoef(float);

Vector3 Gyroscope::SetGyroOffset();
Vector3 Gyroscope::SetAccelOffset();
Vector3 Gyroscope::GetGyroOffset();
Vector3 Gyroscope::GetAccelOffset();

Vector3 Gyroscope::GetAccel();
Vector3 Gyroscope::GetAccelAngle();

Vector3 Gyroscope::GetAngle();

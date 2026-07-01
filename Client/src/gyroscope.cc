#include <cmath>
#include <gyroscope.hh>
#include <i2c_device.hh>

#include <cstdint>
#include <unistd.h>
#include <chrono>

// Credits to https://github.com/rfetick/MPU6050_light

#define MPU6050_SMPLRT_DIV_REGISTER   0x19
#define MPU6050_CONFIG_REGISTER       0x1a
#define MPU6050_GYRO_CONFIG_REGISTER  0x1b
#define MPU6050_ACCEL_CONFIG_REGISTER 0x1c
#define MPU6050_PWR_MGMT_1_REGISTER   0x6b

#define MPU6050_GYRO_OUT_REGISTER     0x43
#define MPU6050_ACCEL_OUT_REGISTER    0x3B

#define RAD_2_DEG             57.29578
#define CALIB_OFFSET_NUM_TIMES   500

#define DEFAULT_GYRO_COEFF    0.98

Gyroscope::Gyroscope(int i2cAdapter) : I2cDevice(i2cAdapter, I2C_ADDRESS) {}

bool Gyroscope::open() {
    if(!static_cast<I2cDevice*>(this)->open())
        return false;

    writeReg(MPU6050_PWR_MGMT_1_REGISTER, 0x01); // power on
    writeReg(MPU6050_SMPLRT_DIV_REGISTER, 0x00);
    writeReg(MPU6050_CONFIG_REGISTER, 0x00);

    setGyroConfig(1);
    setAccelConfig(1);

    return true;
}


uint8_t Gyroscope::setGyroConfig(int c) {
    if(c < 0 || c > 3)
        return 0;

    // [c] [reg] [lsb-to-degsec] [max speed]
    // 0 -> 0x00 131.0 250  deg/s
    // 1 -> 0x08  65.5 500  deg/s
    // 2 -> 0x10  32.8 1000 deg/s
    // 3 -> 0x18  16.4 2000 deg/s
    writeReg(MPU6050_GYRO_CONFIG_REGISTER, (c & 0x03) << 3);
    gyro_lsb_to_degsec_ = 131.0f / (1 << c);

    return 1;
}

uint8_t Gyroscope::setAccelConfig(int c) {
    if(c < 0 || c > 3)
        return 0;

    // [c] [reg] [lsb-to-g] [G-range]
    // 0 -> 0x00 16384.0    +-2g
    // 1 -> 0x08 8192.0     +-4g
    // 2 -> 0x10 4096.0     +-8g
    // 3 -> 0x18 2048.0     +-16g
    writeReg(MPU6050_ACCEL_CONFIG_REGISTER, (c & 0x03) << 3);
    accel_lsb_to_g_ = 2 << (13 - c);
    return 1;
}

void Gyroscope::readRawData() {
    uint8_t buffer[14] = { MPU6050_ACCEL_OUT_REGISTER };
    writeRead(buffer, 1, buffer, 14);

    accel = {
        .X = static_cast<int16_t>(buffer[0] << 8 | buffer[1]) / accel_lsb_to_g_ - offset_accel_.X,
        .Y = static_cast<int16_t>(buffer[2] << 8 | buffer[3]) / accel_lsb_to_g_ - offset_accel_.Y,
        .Z = static_cast<int16_t>(buffer[4] << 8 | buffer[5]) * (is_upside_down_ ? -1 : +1) / accel_lsb_to_g_ - offset_accel_.Z,
    };

    temp = (static_cast<int16_t>(buffer[6] << 8 | buffer[7]) + 12412.0) / 340.0f;

    gyro = {
        .X = static_cast<int16_t>(buffer[8] << 8 | buffer[9]) / gyro_lsb_to_degsec_ - offset_gyro_.X,
        .Y = static_cast<int16_t>(buffer[10] << 8 | buffer[11]) / gyro_lsb_to_degsec_ - offset_gyro_.Y,
        .Z = static_cast<int16_t>(buffer[12] << 8 | buffer[13]) / gyro_lsb_to_degsec_ - offset_gyro_.Z,
    };
}

void Gyroscope::calibrate() {
    offset_accel_ = {};
    offset_gyro_ = {};

    Vector3
        sum_gyro = {},
        sum_accel = {};

    for(int i =0; i < CALIB_OFFSET_NUM_TIMES; i++) {
        readRawData();

        sum_gyro += gyro;
        sum_accel += accel;

        usleep(2'500);
    }

    sum_gyro /= CALIB_OFFSET_NUM_TIMES;
    sum_accel /= CALIB_OFFSET_NUM_TIMES;

    offset_gyro_ = sum_gyro;
    offset_accel_ = sum_accel;
    offset_accel_.Z -= is_upside_down_ ? -1 : 1;
}

double getDeltaTime(std::chrono::steady_clock::time_point& last_time) {
    std::chrono::time_point current = std::chrono::steady_clock::now();
    std::chrono::duration<double> delta = current - last_time;
    last_time = current;
    return delta.count();
}


float Wrap(float angle, float limit) {
    float period = limit * 2.0f;
    angle = std::fmod(angle, period);
    if(angle >  limit) angle -= period;
    if(angle < -limit) angle += period;
    return angle;
}

void Gyroscope::calculateAngles(double dt) {
    float accel_angle_x = std::atan2(accel.Y, sqrt(accel.X * accel.X + accel.Z * accel.Z)) * RAD_2_DEG;
    float accel_angle_y = std::atan2(-accel.X, sqrt(accel.Y * accel.Y + accel.Z * accel.Z)) * RAD_2_DEG;

    if(angle_unset_) {
        angle = { accel_angle_x, accel_angle_y, 0.0f};
        angle_unset_ = false;
        return;
    }

    float gyro_predicted_x = (angle.X + (gyro.X * dt));
    float gyro_predicted_y = (angle.Y + (gyro.Y * dt));

    angle = {
        .X = Wrap(gyro_predicted_x + (1.0f - DEFAULT_GYRO_COEFF) * Wrap(accel_angle_x - gyro_predicted_x, 180.0f), 180.0f),
        .Y = Wrap(gyro_predicted_y + (1.0f - DEFAULT_GYRO_COEFF) * Wrap(accel_angle_y - gyro_predicted_y, 180.0f), 180.0f),
        .Z = Wrap(angle.Z + (gyro.Z * dt), 180.0f)
    };
}

void Gyroscope::update() {
    double deltaTime = getDeltaTime(last_time_);
    readRawData();
    calculateAngles(deltaTime);
}

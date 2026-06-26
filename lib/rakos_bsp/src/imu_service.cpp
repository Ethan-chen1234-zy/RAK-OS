#include <rakos/imu_service.h>
#include <rakos/pin_config.h>

#if RAKOS_HAS_IMU

#include <rakos/i2c_bus.h>
#include <Wire.h>
#include <SensorQMI8658.hpp>

static SensorQMI8658 g_qmi;

bool ImuService::begin() {
    if (!g_qmi.begin(Wire, QMI8658_I2C_ADDR)) {
        Serial.println("[BSP] QMI8658 init failed");
        ready_ = false;
        return false;
    }

    g_qmi.configAccelerometer(SensorQMI8658::ACC_RANGE_4G,
                              SensorQMI8658::ACC_ODR_250Hz,
                              SensorQMI8658::LPF_MODE_0);
    g_qmi.enableAccelerometer();

    g_qmi.configGyroscope(SensorQMI8658::GYR_RANGE_512DPS,
                          SensorQMI8658::GYR_ODR_224_2Hz,
                          SensorQMI8658::LPF_MODE_0);
    g_qmi.enableGyroscope();

    ready_ = true;
    Serial.println("[BSP] QMI8658 ready");
    return true;
}

bool ImuService::update(ImuSample &sample) {
    sample = {};
    if (!ready_ || !g_qmi.getDataReady()) {
        return false;
    }

    rakos::I2cLockGuard lock(50);
    if (!lock.locked()) {
        return false;
    }

    sample.accel_valid = g_qmi.getAccelerometer(sample.ax, sample.ay, sample.az);
    sample.gyro_valid = g_qmi.getGyroscope(sample.gx, sample.gy, sample.gz);
    return sample.accel_valid || sample.gyro_valid;
}

#else  /* !RAKOS_HAS_IMU */

bool ImuService::begin() {
    ready_ = false;
    return false;
}

bool ImuService::update(ImuSample &sample) {
    sample = {};
    return false;
}

#endif /* RAKOS_HAS_IMU */

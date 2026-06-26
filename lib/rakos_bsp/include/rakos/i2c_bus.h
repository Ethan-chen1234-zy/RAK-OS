#pragma once

#include <Arduino.h>

namespace rakos {

/** Call once at boot before any I2C device. Creates the bus mutex. */
void i2cBusBegin();

/** Reset the shared Wire bus after a driver mis-initializes it. */
void i2cBusRecover();

/** Quick probe: returns true when the bus can reach `addr`. */
bool i2cBusProbe(uint8_t addr);

/** Log all responding 7-bit addresses (debug). */
void i2cBusScan();

bool i2cLock(uint32_t timeout_ms = 50);
void i2cUnlock();

/** RAII guard for shared I2C (touch, PMIC, IMU, IO expander). */
class I2cLockGuard {
public:
    explicit I2cLockGuard(uint32_t timeout_ms = 50) : locked_(i2cLock(timeout_ms)) {}
    ~I2cLockGuard() {
        if (locked_) {
            i2cUnlock();
        }
    }
    bool locked() const { return locked_; }

private:
    bool locked_;
};

}  // namespace rakos

#include <rakos/i2c_bus.h>
#include <rakos/pin_config.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#if !defined(RAKOS_BOARD_WAVESHARE_LCD5) && !defined(RAKOS_BOARD_WAVESHARE_LCD5B)
#include <Wire.h>
#endif

namespace rakos {

static SemaphoreHandle_t g_i2c_mutex = nullptr;

static void ensureI2cMutex() {
    if (!g_i2c_mutex) {
        g_i2c_mutex = xSemaphoreCreateRecursiveMutex();
    }
}

#if defined(RAKOS_BOARD_WAVESHARE_LCD5) || defined(RAKOS_BOARD_WAVESHARE_LCD5B)

void i2cBusBegin() {
    ensureI2cMutex();
}

void i2cBusRecover() {}

bool i2cBusProbe(uint8_t addr) {
    (void)addr;
    return false;
}

void i2cBusScan() {
    Serial.println("[I2C] scan skipped (LCD-5 uses ESP Panel legacy I2C driver)");
}

#else

void i2cBusBegin() {
    ensureI2cMutex();
#if defined(ARDUINO_ARCH_ESP32)
    Wire.setPins(I2C_SDA, I2C_SCL);
#endif
    Wire.begin(I2C_SDA, I2C_SCL);
    Wire.setClock(400000);
}

void i2cBusRecover() {
    I2cLockGuard lock(200);
    if (!lock.locked()) {
        return;
    }
    Wire.end();
    delay(5);
#if defined(ARDUINO_ARCH_ESP32)
    Wire.setPins(I2C_SDA, I2C_SCL);
#endif
    Wire.begin(I2C_SDA, I2C_SCL);
    Wire.setClock(400000);
}

bool i2cBusProbe(uint8_t addr) {
    I2cLockGuard lock(50);
    if (!lock.locked()) {
        return false;
    }
    Wire.beginTransmission(addr);
    return Wire.endTransmission() == 0;
}

void i2cBusScan() {
    I2cLockGuard lock(200);
    if (!lock.locked()) {
        Serial.println("[I2C] scan skipped (lock timeout)");
        return;
    }

    Serial.print("[I2C] bus scan:");
    uint8_t found = 0;
    for (uint8_t addr = 1; addr < 127; ++addr) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
            Serial.printf(" 0x%02X", addr);
            ++found;
        }
    }
    if (found == 0) {
        Serial.print(" (none)");
    }
    Serial.println();
}

#endif

bool i2cLock(uint32_t timeout_ms) {
    if (!g_i2c_mutex) {
        return true;
    }
    return xSemaphoreTakeRecursive(g_i2c_mutex, pdMS_TO_TICKS(timeout_ms)) == pdTRUE;
}

void i2cUnlock() {
    if (g_i2c_mutex) {
        xSemaphoreGiveRecursive(g_i2c_mutex);
    }
}

}  // namespace rakos

#include <rakos/io_expander.h>
#include <rakos/pin_config.h>
#if defined(RAKOS_BOARD_WAVESHARE_LCD5) || defined(RAKOS_BOARD_WAVESHARE_LCD5B)
#include <rakos/lcd5_expander.h>
#else
#include <rakos/i2c_bus.h>
#include <Wire.h>
#endif

namespace rakos {

IoExpander &IoExpander::instance() {
    static IoExpander inst;
    return inst;
}

#if defined(RAKOS_BOARD_WAVESHARE_LCD5) || defined(RAKOS_BOARD_WAVESHARE_LCD5B)

bool IoExpander::begin() {
    ready_ = lcd5_expander_ready();
    return true;
}

void IoExpander::pinMode(uint8_t pin, uint8_t mode) {
    auto *exp = lcd5_expander();
    if (!exp || !lcd5_expander_lock(50)) {
        return;
    }
    if (mode == OUTPUT) {
        exp->pinMode(pin, OUTPUT);
    } else {
        exp->pinMode(pin, INPUT);
    }
    lcd5_expander_unlock();
}

void IoExpander::digitalWrite(uint8_t pin, uint8_t level) {
    auto *exp = lcd5_expander();
    if (!exp || !lcd5_expander_lock(50)) {
        return;
    }
    exp->digitalWrite(pin, level);
    lcd5_expander_unlock();
}

bool IoExpander::digitalRead(uint8_t pin) {
    auto *exp = lcd5_expander();
    if (!exp || !lcd5_expander_lock(50)) {
        return false;
    }
    const bool level = exp->digitalRead(pin) != 0;
    lcd5_expander_unlock();
    return level;
}

void IoExpander::setOutputs(uint8_t value) {
    auto *exp = lcd5_expander();
    if (!exp || !lcd5_expander_lock(50)) {
        return;
    }
    for (uint8_t pin = 0; pin < 8; ++pin) {
        exp->digitalWrite(pin, (value >> pin) & 1u);
    }
    lcd5_expander_unlock();
}

void IoExpander::boardPowerOn() {}

void IoExpander::pulseTouchReset() {}

#else  /* AMOLED TCA9554 */

static constexpr uint8_t kRegInput = 0x00;
static constexpr uint8_t kRegOutput = 0x01;
static constexpr uint8_t kRegConfig = 0x03;

// EXIO0/1/2/7 outputs; others input (matches Waveshare BSP config 0x78).
static constexpr uint8_t kOutputMask =
    (1u << EXPIO_LCD_RESET) | (1u << EXPIO_DSI_PWR_EN) | (1u << EXPIO_TP_RESET) | (1u << EXPIO_SD_CS);

void IoExpander::writeReg(uint8_t reg, uint8_t value) {
    rakos::I2cLockGuard lock(100);
    if (!lock.locked()) {
        return;
    }
    Wire.beginTransmission(IOEXP_I2C_ADDR);
    Wire.write(reg);
    Wire.write(value);
    Wire.endTransmission();
}

uint8_t IoExpander::readReg(uint8_t reg) {
    rakos::I2cLockGuard lock(100);
    if (!lock.locked()) {
        return 0xFF;
    }
    Wire.beginTransmission(IOEXP_I2C_ADDR);
    Wire.write(reg);
    Wire.endTransmission(false);
    Wire.requestFrom(IOEXP_I2C_ADDR, (uint8_t)1);
    return Wire.available() ? Wire.read() : 0xFF;
}

bool IoExpander::begin() {
    const uint8_t probe = readReg(kRegInput);
    if (probe == 0xFF && readReg(kRegConfig) == 0xFF) {
        Serial.println("[BSP] TCA9554 not found");
        ready_ = false;
        return false;
    }

    ready_ = true;
    output_ = 0x00;
    config_ = (uint8_t)(0xFF & ~kOutputMask);
    writeReg(kRegConfig, config_);
    writeReg(kRegOutput, output_);
    return true;
}

void IoExpander::pinMode(uint8_t pin, uint8_t mode) {
    if (!ready_ || pin > 7) {
        return;
    }
    if (mode == OUTPUT) {
        config_ &= ~(1u << pin);
    } else {
        config_ |= (1u << pin);
    }
    writeReg(kRegConfig, config_);
}

void IoExpander::digitalWrite(uint8_t pin, uint8_t level) {
    if (!ready_ || pin > 7) {
        return;
    }
    if (level) {
        output_ |= (1u << pin);
    } else {
        output_ &= ~(1u << pin);
    }
    writeReg(kRegOutput, output_);
}

bool IoExpander::digitalRead(uint8_t pin) {
    if (!ready_ || pin > 7) {
        return false;
    }
    return (readReg(kRegInput) >> pin) & 1u;
}

void IoExpander::setOutputs(uint8_t value) {
    if (!ready_) {
        return;
    }
    output_ = value;
    writeReg(kRegOutput, output_);
}

void IoExpander::boardPowerOn() {
    if (!ready_) {
        return;
    }

    // Waveshare ESP-IDF BSP: staged AMOLED power-up via TCA9554.
    setOutputs(1u << EXPIO_SD_CS);
    delay(20);
    setOutputs((1u << EXPIO_DSI_PWR_EN) | (1u << EXPIO_SD_CS));
    delay(20);
    setOutputs((1u << EXPIO_DSI_PWR_EN) | (1u << EXPIO_TP_RESET) | (1u << EXPIO_SD_CS));
    delay(20);
    setOutputs(kOutputMask);
    delay(150);
}

void IoExpander::pulseTouchReset() {
    if (!ready_) {
        return;
    }
    digitalWrite(EXPIO_TP_RESET, LOW);
    delay(20);
    digitalWrite(EXPIO_TP_RESET, HIGH);
    delay(80);
}

#endif /* LCD5 vs AMOLED */

}  // namespace rakos

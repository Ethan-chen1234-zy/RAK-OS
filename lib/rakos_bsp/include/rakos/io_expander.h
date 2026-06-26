#pragma once

#include <Arduino.h>
#if !defined(RAKOS_BOARD_WAVESHARE_LCD5) && !defined(RAKOS_BOARD_WAVESHARE_LCD5B)
#include <Wire.h>
#endif
#include <rakos/pin_config.h>

namespace rakos {

/** TCA9554 on I2C — shared by display power, SD CS, PWR button */
class IoExpander {
public:
    static IoExpander &instance();

    bool begin();
    bool ready() const { return ready_; }

    void pinMode(uint8_t pin, uint8_t mode);
    void digitalWrite(uint8_t pin, uint8_t level);
    bool digitalRead(uint8_t pin);

    /** EXIO0/1/2 power + reset sequence (Waveshare demo) */
    void boardPowerOn();

    /** Pulse EXPIO_TP_RESET (Waveshare touch bring-up). */
    void pulseTouchReset();

private:
    IoExpander() = default;

    bool ready_ = false;
    uint8_t output_ = 0xFF;
    uint8_t config_ = 0xFF;

    void writeReg(uint8_t reg, uint8_t value);
    uint8_t readReg(uint8_t reg);
    void setOutputs(uint8_t value);
};

}  // namespace rakos

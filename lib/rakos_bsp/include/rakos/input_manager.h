#pragma once

#include <Arduino.h>
#include <rakos/pin_config.h>

class InputManager {
public:
    bool init();
    void update();

    bool bootPressed() const { return boot_down_; }
    bool pwrPressed() const { return pwr_down_; }

    /** Rising edge on BOOT (GPIO0) */
    bool bootPressedEdge();
    /** Rising edge on PWR (EXIO4) */
    bool pwrPressedEdge();
    /** BOOT held >= hold_ms */
    bool bootLongPress(uint32_t hold_ms = 1500);
    /** PWR held >= hold_ms */
    bool pwrLongPress(uint32_t hold_ms = 2000);

    /** BOOT or PWR: signal user app to return to OS launcher */
    bool exitToOsCombo() const;

private:
    bool boot_down_ = false;
    bool pwr_down_ = false;
    bool boot_prev_ = false;
    bool pwr_prev_ = false;
    bool boot_edge_ = false;
    bool pwr_edge_ = false;
    bool boot_long_fired_ = false;
    bool pwr_long_fired_ = false;
    uint32_t boot_down_since_ = 0;
    uint32_t pwr_down_since_ = 0;
};

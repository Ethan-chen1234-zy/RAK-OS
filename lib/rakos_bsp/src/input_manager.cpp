#include <rakos/input_manager.h>
#include <rakos/io_expander.h>
#include <rakos/i2c_bus.h>

bool InputManager::init() {
#if RAKOS_HAS_BOOT_BUTTON
    pinMode(BUTTON_BOOT, INPUT_PULLUP);
#endif

#if RAKOS_HAS_PWR_BUTTON
    auto &expander = rakos::IoExpander::instance();
    if (expander.ready()) {
        expander.pinMode(EXPIO_PWR_BUTTON, INPUT);
    }
#endif
    return true;
}

void InputManager::update() {
    boot_prev_ = boot_down_;
    pwr_prev_ = pwr_down_;

#if RAKOS_HAS_BOOT_BUTTON
    boot_down_ = digitalRead(BUTTON_BOOT) == LOW;
#else
    boot_down_ = false;
#endif

#if RAKOS_HAS_PWR_BUTTON
    auto &expander = rakos::IoExpander::instance();
    if (expander.ready()) {
        rakos::I2cLockGuard lock(50);
        if (lock.locked()) {
            pwr_down_ = !expander.digitalRead(EXPIO_PWR_BUTTON);
        } else {
            pwr_down_ = false;
        }
    } else {
        pwr_down_ = false;
    }
#else
    pwr_down_ = false;
#endif

    const uint32_t now = millis();

    boot_edge_ = false;
    pwr_edge_ = false;

    if (boot_down_ && !boot_prev_) {
        boot_down_since_ = now;
        boot_long_fired_ = false;
    }
    if (pwr_down_ && !pwr_prev_) {
        pwr_down_since_ = now;
        pwr_long_fired_ = false;
    }

    // Click on release after >= 40 ms (Waveshare PWR is a pulse on EXIO4).
    if (!boot_down_ && boot_prev_ && (now - boot_down_since_) >= 40) {
        boot_edge_ = true;
    }
    if (!pwr_down_ && pwr_prev_ && (now - pwr_down_since_) >= 40) {
        pwr_edge_ = true;
    }

    if (!boot_down_) {
        boot_long_fired_ = false;
    }
    if (!pwr_down_) {
        pwr_long_fired_ = false;
    }
}

bool InputManager::bootPressedEdge() {
    const bool edge = boot_edge_;
    boot_edge_ = false;
    return edge;
}

bool InputManager::pwrPressedEdge() {
    const bool edge = pwr_edge_;
    pwr_edge_ = false;
    return edge;
}

bool InputManager::bootLongPress(uint32_t hold_ms) {
    if (!boot_down_ || boot_long_fired_) {
        return false;
    }
    if ((millis() - boot_down_since_) >= hold_ms) {
        boot_long_fired_ = true;
        return true;
    }
    return false;
}

bool InputManager::pwrLongPress(uint32_t hold_ms) {
    if (!pwr_down_ || pwr_long_fired_) {
        return false;
    }
    if ((millis() - pwr_down_since_) >= hold_ms) {
        pwr_long_fired_ = true;
        return true;
    }
    return false;
}

bool InputManager::exitToOsCombo() const {
    return boot_down_ || pwr_down_;
}

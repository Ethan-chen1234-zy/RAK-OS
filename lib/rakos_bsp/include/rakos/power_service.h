#pragma once

#include <Arduino.h>

struct PowerStatus {
    bool ready = false;
    bool battery_connected = false;
    bool charging = false;
    bool vbus_in = false;
    uint8_t battery_percent = 0;
    uint16_t battery_mv = 0;
    uint16_t vbus_mv = 0;
    uint16_t system_mv = 0;
};

class PowerService {
public:
    bool begin();
    /** Re-init after Wire bus recovery (SD mount, touch reset). */
    bool rebegin();
    bool ready() const { return ready_; }

    void update();
    const PowerStatus &status() const { return status_; }

    /** AXP2101 soft shutdown — cuts system power (battery required). */
    void shutdown();

private:
    bool ready_ = false;
    PowerStatus status_;
    uint32_t last_read_ms_ = 0;
};

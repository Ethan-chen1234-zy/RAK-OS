#pragma once

#include <Arduino.h>

namespace rakos {

/** Confirm ota_1 boot after grace period (cancels ESP-IDF rollback to ota_0). */
class AppBootConfirm {
public:
    static void tick(uint32_t app_ready_ms, uint32_t grace_ms = 3000);
};

}  // namespace rakos

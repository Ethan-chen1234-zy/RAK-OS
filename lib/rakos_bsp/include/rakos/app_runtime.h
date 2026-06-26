#pragma once

#include <Arduino.h>
#include <lvgl.h>

class DisplayManager;
class InputManager;

namespace rakos {

/** Board-aware display/input bring-up for user apps (ota_1). */
class AppRuntime {
public:
    static bool beginHardware(DisplayManager &display, InputManager &input);
    static void pumpUi(DisplayManager &display);

    /** Call once from setup() after beginHardware() to build LVGL UI safely. */
    static void buildUiLocked(void (*build_fn)());

    /** On LCD-5 (no BOOT key): on-screen button to return to RAKOS. */
    static void addExitButton(lv_obj_t *parent);

    /** BOOT long-press when available; always false on LCD-5. */
    static bool bootExitRequested(InputManager &input, uint32_t app_ready_ms, uint32_t grace_ms, uint32_t hold_ms);
};

}  // namespace rakos

#include <rakos/app_runtime.h>
#include <rakos/boot_manager.h>
#include <rakos/display_manager.h>
#include <rakos/input_manager.h>
#include <rakos/io_expander.h>
#include <rakos/pin_config.h>

#if !defined(RAKOS_BOARD_WAVESHARE_LCD5) && !defined(RAKOS_BOARD_WAVESHARE_LCD5B)
#include <rakos/i2c_bus.h>
#endif

#if defined(RAKOS_BOARD_WAVESHARE_LCD5) || defined(RAKOS_BOARD_WAVESHARE_LCD5B)
#include "lvgl_v8_port.h"
#endif

namespace rakos {

static void onExitButtonClicked(lv_event_t *e) {
    (void)e;
    Serial.println("[APP] Exit -> RAKOS (ota_0)");
    if (BootManager::setNextBootOs()) {
        BootManager::reboot();
    }
}

bool AppRuntime::beginHardware(DisplayManager &display, InputManager &input) {
#if defined(RAKOS_BOARD_WAVESHARE_LCD5) || defined(RAKOS_BOARD_WAVESHARE_LCD5B)
    (void)IoExpander::instance().begin();
    input.init();
    if (!display.init()) {
        return false;
    }
    Serial.println("[APP] LCD-5 display ready (lvgl_port task owns timer)");
    return true;
#else
    i2cBusBegin();
    auto &expander = IoExpander::instance();
    if (!expander.begin()) {
        Serial.println("[APP] IO expander init failed");
        return false;
    }
    expander.boardPowerOn();
    delay(100);

    input.init();
    (void)display.initTouch();
    if (!display.init()) {
        return false;
    }
    display.startLvglTick();
    return true;
#endif
}

void AppRuntime::pumpUi(DisplayManager &display) {
#if defined(RAKOS_BOARD_WAVESHARE_LCD5) || defined(RAKOS_BOARD_WAVESHARE_LCD5B)
    (void)display;
    vTaskDelay(pdMS_TO_TICKS(5));
#else
    display.runLvgl();
#endif
}

void AppRuntime::buildUiLocked(void (*build_fn)()) {
    if (!build_fn) {
        return;
    }
#if defined(RAKOS_BOARD_WAVESHARE_LCD5) || defined(RAKOS_BOARD_WAVESHARE_LCD5B)
    if (lvgl_port_lock(-1)) {
        build_fn();
        lvgl_port_unlock();
    }
#else
    build_fn();
#endif
}

void AppRuntime::addExitButton(lv_obj_t *parent) {
#if RAKOS_HAS_BOOT_BUTTON
    (void)parent;
    return;
#endif

    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, 148, 44);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_RIGHT, -16, -16);
    lv_obj_set_style_radius(btn, 8, 0);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x333333), 0);
    lv_obj_add_event_cb(btn, onExitButtonClicked, LV_EVENT_CLICKED, nullptr);

    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, "RAKOS");
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xFF7F1F), 0);
    lv_obj_center(lbl);
}

bool AppRuntime::bootExitRequested(InputManager &input, uint32_t app_ready_ms, uint32_t grace_ms,
                                   uint32_t hold_ms) {
#if !RAKOS_HAS_BOOT_BUTTON
    (void)input;
    (void)app_ready_ms;
    (void)grace_ms;
    (void)hold_ms;
    return false;
#else
    if ((millis() - app_ready_ms) < grace_ms) {
        return false;
    }
    return input.bootLongPress(hold_ms);
#endif
}

}  // namespace rakos

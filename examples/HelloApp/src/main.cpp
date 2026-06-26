/**
 * RAKOS HelloApp — demo user application (ota_1 @ 0x400000)
 *
 * AMOLED: BOOT hold 1.5s -> RAKOS
 * LCD-5:  tap "RAKOS" button (bottom-right)
 */
#include <Arduino.h>
#include <lvgl.h>

#include <rakos/app_runtime.h>
#include <rakos/boot_manager.h>
#include <rakos/display_manager.h>
#include <rakos/input_manager.h>
#include <rakos/lvgl_compat.h>
#include <rakos/pin_config.h>

static DisplayManager display;
static InputManager input;
static lv_obj_t *counter_label = nullptr;
static uint32_t uptime_s = 0;
static uint32_t last_tick_ms = 0;
static uint32_t app_ready_ms = 0;

static constexpr uint32_t kExitGraceMs = 3000;
static constexpr uint32_t kExitBootHoldMs = 1500;

static void build_ui() {
    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x001018), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "Hello RAKOS");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFF7F1F), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 48);

    lv_obj_t *sub = lv_label_create(scr);
#if RAKOS_HAS_BOOT_BUTTON
    lv_label_set_text(sub, "Demo app (ota_1)\n\nBOOT hold 1.5s -> RAKOS");
#else
    lv_label_set_text(sub, "Demo app (ota_1)\nLCD-5 800x480\nTap RAKOS to exit");
#endif
    lv_obj_set_style_text_color(sub, lv_color_hex(0xB0AAA4), 0);
    lv_obj_set_style_text_align(sub, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(sub, LV_ALIGN_CENTER, 0, -20);

    counter_label = lv_label_create(scr);
    lv_label_set_text(counter_label, "0 s");
    lv_obj_set_style_text_color(counter_label, lv_color_hex(0x00FF88), 0);
    lv_obj_align(counter_label, LV_ALIGN_BOTTOM_MID, 0, -80);

    rakos::AppRuntime::addExitButton(scr);
}

void setup() {
    Serial.begin(115200);
    delay(300);
    Serial.println();
    Serial.println("=== HelloApp (RAKOS demo, ota_1) ===");

    BootManager::begin();
    if (!BootManager::isRunningAppSlot()) {
        Serial.println("[WARN] Not running from ota_1 — flash to 0x400000 or install from SD");
    }

    if (!rakos::AppRuntime::beginHardware(display, input)) {
        Serial.println("[FATAL] Display init failed");
        for (;;) {
            delay(1000);
        }
    }

    rakos::AppRuntime::buildUiLocked(build_ui);

    last_tick_ms = millis();
    app_ready_ms = last_tick_ms;
    Serial.println("[OK] HelloApp running");
}

void loop() {
    input.update();

    if (rakos::AppRuntime::bootExitRequested(input, app_ready_ms, kExitGraceMs, kExitBootHoldMs)) {
        Serial.println("[APP] Request boot to RAKOS (ota_0)");
        if (BootManager::setNextBootOs()) {
            BootManager::reboot();
        }
    }

    const uint32_t now = millis();
    if (counter_label && (now - last_tick_ms) >= 1000) {
        last_tick_ms = now;
        ++uptime_s;
        lv_label_set_text_fmt(counter_label, "%lu s", (unsigned long)uptime_s);
    }

    rakos::AppRuntime::pumpUi(display);
}

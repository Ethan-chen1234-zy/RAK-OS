/** RAKOS MeterApp — arc gauge demo (ota_1 @ 0x400000) */
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
static lv_obj_t *arc = nullptr;
static lv_obj_t *value_label = nullptr;
static int gauge_val = 0;
static uint32_t app_ready_ms = 0;
static uint32_t last_anim_ms = 0;

static constexpr uint32_t kExitGraceMs = 3000;
static constexpr uint32_t kExitBootHoldMs = 1500;

static int arcSize() {
#if defined(RAKOS_BOARD_WAVESHARE_LCD5) || defined(RAKOS_BOARD_WAVESHARE_LCD5B)
    return 280;
#else
    return 200;
#endif
}

static void build_ui() {
    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x0A1420), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "Meter Demo");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFF7F1F), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 36);

    const int sz = arcSize();
    arc = lv_arc_create(scr);
    lv_obj_set_size(arc, sz, sz);
    lv_arc_set_range(arc, 0, 100);
    lv_arc_set_value(arc, 0);
    lv_obj_remove_style(arc, nullptr, LV_PART_KNOB);
    lv_obj_clear_flag(arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_width(arc, 14, LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc, 14, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(arc, lv_color_hex(0x303840), LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc, lv_color_hex(0x00E5FF), LV_PART_INDICATOR);
    lv_obj_align(arc, LV_ALIGN_CENTER, 0, -10);

    value_label = lv_label_create(scr);
    lv_label_set_text(value_label, "0 %");
    lv_obj_set_style_text_color(value_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(value_label, &lv_font_montserrat_24, 0);
    lv_obj_align(value_label, LV_ALIGN_CENTER, 0, -10);

    rakos::AppRuntime::addExitButton(scr);
}

void setup() {
    Serial.begin(115200);
    delay(300);
    Serial.println("=== MeterApp (RAKOS demo) ===");

    BootManager::begin();
    if (!rakos::AppRuntime::beginHardware(display, input)) {
        for (;;) {
            delay(1000);
        }
    }
    rakos::AppRuntime::buildUiLocked(build_ui);
    app_ready_ms = millis();
    last_anim_ms = app_ready_ms;
}

void loop() {
    input.update();
    if (rakos::AppRuntime::bootExitRequested(input, app_ready_ms, kExitGraceMs, kExitBootHoldMs)) {
        if (BootManager::setNextBootOs()) {
            BootManager::reboot();
        }
    }

    const uint32_t now = millis();
    if (arc && (now - last_anim_ms) >= 80) {
        last_anim_ms = now;
        gauge_val += 2;
        if (gauge_val > 100) {
            gauge_val = 0;
        }
        lv_arc_set_value(arc, gauge_val);
        if (value_label) {
            lv_label_set_text_fmt(value_label, "%d %%", gauge_val);
        }
    }

    rakos::AppRuntime::pumpUi(display);
}

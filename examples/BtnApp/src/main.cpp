/** RAKOS BtnApp — colored button demo (ota_1 @ 0x400000) */
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
static lv_obj_t *status_label = nullptr;
static uint32_t app_ready_ms = 0;

static constexpr uint32_t kExitGraceMs = 3000;
static constexpr uint32_t kExitBootHoldMs = 1500;

static void btn_event_cb(lv_event_t *e) {
    const char *txt = static_cast<const char *>(lv_event_get_user_data(e));
    if (status_label) {
        lv_label_set_text_fmt(status_label, "Tapped: %s", txt ? txt : "?");
    }
}

static void build_ui() {
    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x101820), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "Button Demo");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFF7F1F), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 36);

    static const struct {
        const char *text;
        uint32_t color;
    } kBtns[] = {{"Start", 0x00C853}, {"Stop", 0xD50000}, {"Info", 0x2962FF}};

    for (size_t i = 0; i < sizeof(kBtns) / sizeof(kBtns[0]); ++i) {
        lv_obj_t *btn = lv_button_create(scr);
        lv_obj_set_size(btn, 200, 48);
        lv_obj_align(btn, LV_ALIGN_TOP_MID, 0, static_cast<lv_coord_t>(100 + i * 58));
        lv_obj_set_style_radius(btn, 12, 0);
        lv_obj_set_style_bg_color(btn, lv_color_hex(kBtns[i].color), 0);
        lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_CLICKED, const_cast<void *>(static_cast<const void *>(kBtns[i].text)));

        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, kBtns[i].text);
        lv_obj_center(lbl);
    }

    status_label = lv_label_create(scr);
    lv_label_set_text(status_label, "Tap a button");
    lv_obj_set_style_text_color(status_label, lv_color_hex(0xB0AAA4), 0);
    lv_obj_align(status_label, LV_ALIGN_BOTTOM_MID, 0, -72);

    rakos::AppRuntime::addExitButton(scr);
}

void setup() {
    Serial.begin(115200);
    delay(300);
    Serial.println("=== BtnApp (RAKOS demo) ===");

    BootManager::begin();
    if (!rakos::AppRuntime::beginHardware(display, input)) {
        for (;;) {
            delay(1000);
        }
    }
    rakos::AppRuntime::buildUiLocked(build_ui);
    app_ready_ms = millis();
}

void loop() {
    input.update();
    if (rakos::AppRuntime::bootExitRequested(input, app_ready_ms, kExitGraceMs, kExitBootHoldMs)) {
        if (BootManager::setNextBootOs()) {
            BootManager::reboot();
        }
    }
    rakos::AppRuntime::pumpUi(display);
}

/** RAKOS SliderApp — brightness + volume (ota_1 @ 0x400000) */
#include <Arduino.h>
#include <lvgl.h>

#include <rakos/app_runtime.h>
#include <rakos/audio_service.h>
#include <rakos/boot_manager.h>
#include <rakos/display_manager.h>
#include <rakos/input_manager.h>
#include <rakos/lvgl_compat.h>
#include <rakos/pin_config.h>

static DisplayManager display;
static InputManager input;
#if RAKOS_HAS_AUDIO
static AudioService audio;
#endif

static lv_obj_t *bright_label = nullptr;
static lv_obj_t *vol_label = nullptr;
static lv_obj_t *hint_label = nullptr;
static uint32_t app_ready_ms = 0;
static uint32_t last_beep_ms = 0;

static constexpr uint32_t kExitGraceMs = 3000;
static constexpr uint32_t kExitBootHoldMs = 1500;

static void bright_slider_cb(lv_event_t *e) {
    const int32_t val = lv_slider_get_value(static_cast<lv_obj_t *>(lv_event_get_target(e)));
    if (bright_label) {
        lv_label_set_text_fmt(bright_label, "%ld", (long)val);
    }
    display.setBrightnessPercent(static_cast<uint8_t>(val), true);
}

static void vol_slider_cb(lv_event_t *e) {
    const int32_t val = lv_slider_get_value(static_cast<lv_obj_t *>(lv_event_get_target(e)));
    if (vol_label) {
        lv_label_set_text_fmt(vol_label, "%ld", (long)val);
    }
#if RAKOS_HAS_AUDIO
    if (audio.ready()) {
        audio.setVolume(static_cast<uint8_t>(val));
        const uint32_t now = millis();
        if ((now - last_beep_ms) >= 350) {
            last_beep_ms = now;
            audio.beep(880, 36);
        }
    }
#else
    (void)last_beep_ms;
#endif
}

static void build_ui() {
    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x121018), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "Display & Audio");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFF7F1F), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 36);

    lv_obj_t *lbl1 = lv_label_create(scr);
    lv_label_set_text(lbl1, "Brightness");
    lv_obj_set_style_text_color(lbl1, lv_color_hex(0xB0AAA4), 0);
    lv_obj_align(lbl1, LV_ALIGN_TOP_LEFT, 24, 100);

    bright_label = lv_label_create(scr);
    lv_label_set_text(bright_label, "80");
    lv_obj_set_style_text_color(bright_label, lv_color_hex(0x00FF88), 0);
    lv_obj_align(bright_label, LV_ALIGN_TOP_RIGHT, -24, 100);

    lv_obj_t *s1 = lv_slider_create(scr);
    lv_obj_set_width(s1, lv_pct(85));
    lv_slider_set_range(s1, 5, 100);
    lv_slider_set_value(s1, display.getBrightnessPercentage(), LV_ANIM_OFF);
    lv_obj_align(s1, LV_ALIGN_TOP_MID, 0, 132);
    lv_obj_add_event_cb(s1, bright_slider_cb, LV_EVENT_VALUE_CHANGED, nullptr);

    lv_obj_t *lbl2 = lv_label_create(scr);
    lv_label_set_text(lbl2, "Volume");
    lv_obj_set_style_text_color(lbl2, lv_color_hex(0xB0AAA4), 0);
    lv_obj_align(lbl2, LV_ALIGN_TOP_LEFT, 24, 190);

    vol_label = lv_label_create(scr);
#if RAKOS_HAS_AUDIO
    lv_label_set_text_fmt(vol_label, "%u", audio.volumePercent());
#else
    lv_label_set_text(vol_label, "N/A");
#endif
    lv_obj_set_style_text_color(vol_label, lv_color_hex(0x00FF88), 0);
    lv_obj_align(vol_label, LV_ALIGN_TOP_RIGHT, -24, 190);

    lv_obj_t *s2 = lv_slider_create(scr);
    lv_obj_set_width(s2, lv_pct(85));
    lv_slider_set_range(s2, 0, 100);
#if RAKOS_HAS_AUDIO
    lv_slider_set_value(s2, audio.volumePercent(), LV_ANIM_OFF);
    lv_obj_clear_state(s2, LV_STATE_DISABLED);
#else
    lv_slider_set_value(s2, 0, LV_ANIM_OFF);
    lv_obj_add_state(s2, LV_STATE_DISABLED);
#endif
    lv_obj_align(s2, LV_ALIGN_TOP_MID, 0, 222);
    lv_obj_add_event_cb(s2, vol_slider_cb, LV_EVENT_VALUE_CHANGED, nullptr);

    hint_label = lv_label_create(scr);
#if RAKOS_HAS_AUDIO
    lv_label_set_text(hint_label, "Values saved to NVS (shared with RAKOS)");
#else
    lv_label_set_text(hint_label, "Brightness only (no audio on this board)");
#endif
    lv_obj_set_style_text_color(hint_label, lv_color_hex(0x666666), 0);
    lv_obj_align(hint_label, LV_ALIGN_BOTTOM_MID, 0, -72);

    rakos::AppRuntime::addExitButton(scr);
}

void setup() {
    Serial.begin(115200);
    delay(300);
    Serial.println("=== SliderApp (RAKOS demo) ===");

    BootManager::begin();
    if (!rakos::AppRuntime::beginHardware(display, input)) {
        for (;;) {
            delay(1000);
        }
    }

#if RAKOS_HAS_AUDIO
    if (!audio.begin()) {
        Serial.println("[APP] Audio init failed — volume slider disabled");
    }
#endif

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
    rakos::AppRuntime::pumpUi(display, app_ready_ms);
}

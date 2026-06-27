/** RAKOS BtnApp — buttons + audio + timer (ota_1 @ 0x400000) */
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

static lv_obj_t *status_label = nullptr;
static uint32_t app_ready_ms = 0;

static bool timer_running = false;
static uint32_t timer_elapsed_ms = 0;
static uint32_t timer_start_ms = 0;
static uint32_t last_timer_ui_ms = 0;

static constexpr uint32_t kExitGraceMs = 3000;
static constexpr uint32_t kExitBootHoldMs = 1500;

static uint32_t timerNowMs() {
    if (!timer_running) {
        return timer_elapsed_ms;
    }
    return timer_elapsed_ms + (millis() - timer_start_ms);
}

static void updateTimerStatus() {
    if (!status_label) {
        return;
    }
    const uint32_t ms = timerNowMs();
    const uint32_t sec = ms / 1000U;
    const uint32_t min = sec / 60U;
    const uint32_t rem = sec % 60U;
    lv_label_set_text_fmt(status_label, "Timer: %02lu:%02lu %s", (unsigned long)min, (unsigned long)rem,
                          timer_running ? "(running)" : "(paused)");
}

static void on_start(lv_event_t *e) {
    (void)e;
#if RAKOS_HAS_AUDIO
    if (audio.ready()) {
        audio.beep(920, 40);
    }
#endif
    if (!timer_running) {
        timer_start_ms = millis();
        timer_running = true;
    }
    updateTimerStatus();
}

static void on_stop(lv_event_t *e) {
    (void)e;
#if RAKOS_HAS_AUDIO
    if (audio.ready()) {
        audio.beep(440, 50);
    }
#endif
    if (timer_running) {
        timer_elapsed_ms = timerNowMs();
        timer_running = false;
    } else {
        timer_elapsed_ms = 0;
    }
    updateTimerStatus();
}

static void on_info(lv_event_t *e) {
    (void)e;
#if RAKOS_HAS_AUDIO
    if (audio.ready()) {
        audio.beep(660, 36);
    }
#endif
    const PartitionSummary s = BootManager::getSummary();
    const char *slot = BootManager::isRunningAppSlot() ? "ota_1 (app)" : "ota_0 (OS)";
    if (status_label) {
        lv_label_set_text_fmt(status_label,
                              "%s\nboot=%s  ota1=%s",
                              slot,
                              s.boot ? s.boot->label : "?",
                              BootManager::ota1HasFirmware() ? "ready" : "empty");
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
        lv_event_cb_t cb;
    } kBtns[] = {{"Start", 0x00C853, on_start}, {"Stop", 0xD50000, on_stop}, {"Info", 0x2962FF, on_info}};

    for (size_t i = 0; i < sizeof(kBtns) / sizeof(kBtns[0]); ++i) {
        lv_obj_t *btn = lv_button_create(scr);
        lv_obj_set_size(btn, 200, 48);
        lv_obj_align(btn, LV_ALIGN_TOP_MID, 0, static_cast<lv_coord_t>(100 + i * 58));
        lv_obj_set_style_radius(btn, 12, 0);
        lv_obj_set_style_bg_color(btn, lv_color_hex(kBtns[i].color), 0);
        lv_obj_add_event_cb(btn, kBtns[i].cb, LV_EVENT_CLICKED, nullptr);

        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, kBtns[i].text);
        lv_obj_center(lbl);
    }

    status_label = lv_label_create(scr);
    lv_label_set_text(status_label, "Start = run timer  |  Info = boot slot");
    lv_obj_set_style_text_color(status_label, lv_color_hex(0xB0AAA4), 0);
    lv_obj_set_style_text_align(status_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(status_label, lv_pct(90));
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

#if RAKOS_HAS_AUDIO
    (void)audio.begin();
#endif

    rakos::AppRuntime::buildUiLocked(build_ui);
    app_ready_ms = millis();
    last_timer_ui_ms = app_ready_ms;
}

void loop() {
    input.update();
    if (rakos::AppRuntime::bootExitRequested(input, app_ready_ms, kExitGraceMs, kExitBootHoldMs)) {
        if (BootManager::setNextBootOs()) {
            BootManager::reboot();
        }
    }

    if (timer_running) {
        const uint32_t now = millis();
        if ((now - last_timer_ui_ms) >= 250) {
            last_timer_ui_ms = now;
            updateTimerStatus();
        }
    }

    rakos::AppRuntime::pumpUi(display, app_ready_ms);
}

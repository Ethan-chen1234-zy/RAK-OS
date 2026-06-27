/** RAKOS MeterApp — IMU tilt gauge (ota_1 @ 0x400000) */
#include <Arduino.h>
#include <lvgl.h>
#include <math.h>

#include <rakos/app_runtime.h>
#include <rakos/boot_manager.h>
#include <rakos/display_manager.h>
#include <rakos/imu_service.h>
#include <rakos/input_manager.h>
#include <rakos/lvgl_compat.h>
#include <rakos/pin_config.h>

static DisplayManager display;
static InputManager input;
#if RAKOS_HAS_IMU
static ImuService imu;
#endif

static lv_obj_t *arc = nullptr;
static lv_obj_t *value_label = nullptr;
static lv_obj_t *mode_label = nullptr;
static int gauge_val = 0;
static uint32_t app_ready_ms = 0;
static uint32_t last_anim_ms = 0;

static constexpr uint32_t kExitGraceMs = 3000;
static constexpr uint32_t kExitBootHoldMs = 1500;

enum class MeterMode : uint8_t { kTilt, kDemo };

static MeterMode meter_mode = MeterMode::kTilt;

static int arcSize() {
#if defined(RAKOS_BOARD_WAVESHARE_LCD5) || defined(RAKOS_BOARD_WAVESHARE_LCD5B)
    return 280;
#else
    return 200;
#endif
}

static int tiltFromImu(const ImuSample &s) {
    if (!s.accel_valid) {
        return -1;
    }
    const float ax = s.ax;
    const float ay = s.ay;
    const float az = s.az;
    const float pitch = atan2f(ax, sqrtf(ay * ay + az * az)) * 180.0f / (float)M_PI;
    const int val = (int)(fabsf(pitch) * 100.0f / 45.0f);
    return val > 100 ? 100 : val;
}

static void applyGauge(int val) {
    gauge_val = val;
    if (arc) {
        lv_arc_set_value(arc, val);
    }
    if (value_label) {
        lv_label_set_text_fmt(value_label, "%d %%", val);
    }
}

static void refreshModeLabel() {
    if (!mode_label) {
        return;
    }
#if RAKOS_HAS_IMU
    if (meter_mode == MeterMode::kTilt && imu.ready()) {
        lv_label_set_text(mode_label, "Tilt (tap arc: demo mode)");
    } else if (meter_mode == MeterMode::kTilt) {
        lv_label_set_text(mode_label, "Tilt (IMU unavailable)");
    } else {
        lv_label_set_text(mode_label, "Demo animation (tap arc: tilt)");
    }
#else
    lv_label_set_text(mode_label, "Demo animation (no IMU on board)");
#endif
}

static void arc_click_cb(lv_event_t *e) {
    (void)e;
#if RAKOS_HAS_IMU
    meter_mode = (meter_mode == MeterMode::kTilt) ? MeterMode::kDemo : MeterMode::kTilt;
#else
    meter_mode = MeterMode::kDemo;
#endif
    refreshModeLabel();
}

static void build_ui() {
    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x0A1420), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "Tilt Meter");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFF7F1F), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 36);

    const int sz = arcSize();
    arc = lv_arc_create(scr);
    lv_obj_set_size(arc, sz, sz);
    lv_arc_set_range(arc, 0, 100);
    lv_arc_set_value(arc, 0);
    lv_obj_remove_style(arc, nullptr, LV_PART_KNOB);
    lv_obj_add_flag(arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(arc, arc_click_cb, LV_EVENT_CLICKED, nullptr);
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

    mode_label = lv_label_create(scr);
    lv_obj_set_style_text_color(mode_label, lv_color_hex(0x9A948F), 0);
    lv_obj_align(mode_label, LV_ALIGN_BOTTOM_MID, 0, -72);
    refreshModeLabel();

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

#if RAKOS_HAS_IMU
    if (!imu.begin()) {
        Serial.println("[APP] IMU init failed — demo animation only");
        meter_mode = MeterMode::kDemo;
    }
#else
    meter_mode = MeterMode::kDemo;
#endif

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
    if ((now - last_anim_ms) < 80) {
        rakos::AppRuntime::pumpUi(display, app_ready_ms);
        return;
    }
    last_anim_ms = now;

#if RAKOS_HAS_IMU
    if (meter_mode == MeterMode::kTilt && imu.ready()) {
        ImuSample sample;
        if (imu.update(sample)) {
            const int tilt = tiltFromImu(sample);
            if (tilt >= 0) {
                applyGauge(tilt);
            }
        }
    } else
#endif
    {
        int next = gauge_val + 2;
        if (next > 100) {
            next = 0;
        }
        applyGauge(next);
    }

    rakos::AppRuntime::pumpUi(display, app_ready_ms);
}

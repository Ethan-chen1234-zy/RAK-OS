/**
 * RAKOS ClockApp — analog + digital watch demo (ota_1 @ 0x400000)
 *
 * AMOLED: RTC when present; BOOT hold -> RAKOS
 * LCD-5:  soft clock; tap RAKOS button to exit
 */
#include <Arduino.h>
#include <lvgl.h>
#include <math.h>
#include <time.h>

#include <rakos/app_runtime.h>
#include <rakos/boot_manager.h>
#include <rakos/display_manager.h>
#include <rakos/input_manager.h>
#include <rakos/lvgl_compat.h>
#include <rakos/pin_config.h>

#if !defined(RAKOS_BOARD_WAVESHARE_LCD5) && !defined(RAKOS_BOARD_WAVESHARE_LCD5B)
#include <Wire.h>
#include <rakos/i2c_bus.h>
#include <SensorRtcHelper.hpp>
#endif

static DisplayManager display;
static InputManager input;
#if !defined(RAKOS_BOARD_WAVESHARE_LCD5) && !defined(RAKOS_BOARD_WAVESHARE_LCD5B)
static SensorRtcHelper rtc;
#endif

static lv_obj_t *digital_label = nullptr;
static lv_obj_t *date_label = nullptr;
static lv_obj_t *source_label = nullptr;
static lv_obj_t *hour_hand = nullptr;
static lv_obj_t *min_hand = nullptr;
static lv_obj_t *sec_hand = nullptr;

static lv_point_precise_t hour_pts[2];
static lv_point_precise_t min_pts[2];
static lv_point_precise_t sec_pts[2];

static bool rtc_ok = false;
static time_t soft_epoch = 0;
static uint32_t soft_millis_base = 0;
static uint32_t last_ui_ms = 0;
static uint32_t app_ready_ms = 0;

static constexpr uint32_t kExitGraceMs = 3000;
static constexpr uint32_t kExitBootHoldMs = 1500;

static const char *kWeekdays[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

static int clockDialSize() {
#if defined(RAKOS_BOARD_WAVESHARE_LCD5) || defined(RAKOS_BOARD_WAVESHARE_LCD5B)
    return 360;
#else
    return 240;
#endif
}

static int parseMonth(const char *mon) {
    static const char *names[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    for (int i = 0; i < 12; ++i) {
        if (mon[0] == names[i][0] && mon[1] == names[i][1] && mon[2] == names[i][2]) {
            return i + 1;
        }
    }
    return 1;
}

static void initSoftClock() {
    char month_s[4] = {};
    int day = 1;
    int year = 2025;
    int hour = 0;
    int minute = 0;
    int second = 0;
    sscanf(__DATE__, "%3s %d %d", month_s, &day, &year);
    sscanf(__TIME__, "%d:%d:%d", &hour, &minute, &second);

    struct tm t = {};
    t.tm_year = year - 1900;
    t.tm_mon = parseMonth(month_s) - 1;
    t.tm_mday = day;
    t.tm_hour = hour;
    t.tm_min = minute;
    t.tm_sec = second;
    soft_epoch = mktime(&t);
    soft_millis_base = millis();
}

static bool readSoftTm(struct tm &out) {
    const time_t now = soft_epoch + (time_t)((millis() - soft_millis_base) / 1000U);
    localtime_r(&now, &out);
    return true;
}

#if !defined(RAKOS_BOARD_WAVESHARE_LCD5) && !defined(RAKOS_BOARD_WAVESHARE_LCD5B)
static bool readRtcTm(struct tm &out) {
    rakos::I2cLockGuard lock(80);
    if (!lock.locked()) {
        return false;
    }

    const RTC_DateTime dt = rtc.getDateTime();
    if (dt.getYear() < 2020) {
        return false;
    }

    memset(&out, 0, sizeof(out));
    out.tm_year = (int)dt.getYear() - 1900;
    out.tm_mon = (int)dt.getMonth() - 1;
    out.tm_mday = (int)dt.getDay();
    out.tm_hour = (int)dt.getHour();
    out.tm_min = (int)dt.getMinute();
    out.tm_sec = (int)dt.getSecond();
    out.tm_wday = (int)dt.getWeek() % 7;
    return true;
}
#endif

static bool getLocalTm(struct tm &out) {
#if !defined(RAKOS_BOARD_WAVESHARE_LCD5) && !defined(RAKOS_BOARD_WAVESHARE_LCD5B)
    if (rtc_ok && readRtcTm(out)) {
        return true;
    }
#endif
    return readSoftTm(out);
}

static void handEndpoint(int cx, int cy, int len, float angle_deg, int &x, int &y) {
    const float rad = angle_deg * (float)M_PI / 180.0f;
    x = cx + (int)(sinf(rad) * (float)len);
    y = cy - (int)(cosf(rad) * (float)len);
}

static void setHand(lv_obj_t *line, lv_point_precise_t *pts, int cx, int cy, int len, float angle_deg,
                    lv_color_t color, int width) {
    if (!line) {
        return;
    }
    pts[0].x = cx;
    pts[0].y = cy;
    int ex = cx;
    int ey = cy;
    handEndpoint(cx, cy, len, angle_deg, ex, ey);
    pts[1].x = ex;
    pts[1].y = ey;
    lv_line_set_points(line, pts, 2);
    lv_obj_set_style_line_color(line, color, 0);
    lv_obj_set_style_line_width(line, width, 0);
    lv_obj_set_style_line_rounded(line, true, 0);
}

static void updateClockFace(const struct tm &t) {
    const int h = t.tm_hour;
    const int m = t.tm_min;
    const int s = t.tm_sec;
    const int dial = clockDialSize();
    const int cx = dial / 2;
    const int cy = dial / 2;

    const float sec_a = (float)s * 6.0f;
    const float min_a = (float)m * 6.0f + (float)s * 0.1f;
    const float hour_a = (float)(h % 12) * 30.0f + (float)m * 0.5f;

    setHand(hour_hand, hour_pts, cx, cy, dial * 44 / 240, hour_a, lv_color_hex(0xFFFAF5), 5);
    setHand(min_hand, min_pts, cx, cy, dial * 66 / 240, min_a, lv_color_hex(0xFFFAF5), 4);
    setHand(sec_hand, sec_pts, cx, cy, dial * 78 / 240, sec_a, lv_color_hex(0xFF7F1F), 2);

    if (digital_label) {
        lv_label_set_text_fmt(digital_label, "%02d:%02d:%02d", h, m, s);
    }
    if (date_label) {
        const char *wday = (t.tm_wday >= 0 && t.tm_wday <= 6) ? kWeekdays[t.tm_wday] : "---";
        lv_label_set_text_fmt(date_label, "%s  %04d-%02d-%02d", wday, t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);
    }
    if (source_label) {
        lv_label_set_text(source_label, rtc_ok ? "RTC" : "Soft clock");
    }
}

static void build_ui() {
    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    const int dial = clockDialSize();

    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "Clock");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFF7F1F), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 24);

    digital_label = lv_label_create(scr);
    lv_label_set_text(digital_label, "--:--:--");
    lv_obj_set_style_text_color(digital_label, lv_color_hex(0xFFFAF5), 0);
    lv_obj_set_style_text_font(digital_label, &lv_font_montserrat_24, 0);
    lv_obj_align(digital_label, LV_ALIGN_TOP_MID, 0, 64);

    date_label = lv_label_create(scr);
    lv_label_set_text(date_label, "---");
    lv_obj_set_style_text_color(date_label, lv_color_hex(0xB0AAA4), 0);
    lv_obj_set_style_text_font(date_label, &lv_font_montserrat_20, 0);
    lv_obj_align(date_label, LV_ALIGN_TOP_MID, 0, 104);

    lv_obj_t *dial_obj = lv_obj_create(scr);
    lv_obj_remove_style_all(dial_obj);
    lv_obj_set_size(dial_obj, dial, dial);
    lv_obj_align(dial_obj, LV_ALIGN_CENTER, 0, 40);
    lv_obj_set_style_radius(dial_obj, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(dial_obj, 2, 0);
    lv_obj_set_style_border_color(dial_obj, lv_color_hex(0x333333), 0);
    lv_obj_set_style_bg_opa(dial_obj, LV_OPA_TRANSP, 0);
    lv_obj_clear_flag(dial_obj, LV_OBJ_FLAG_SCROLLABLE);

    hour_hand = lv_line_create(dial_obj);
    min_hand = lv_line_create(dial_obj);
    sec_hand = lv_line_create(dial_obj);

    lv_obj_t *hub = lv_obj_create(dial_obj);
    lv_obj_remove_style_all(hub);
    lv_obj_set_size(hub, 12, 12);
    lv_obj_set_style_bg_color(hub, lv_color_hex(0xFF7F1F), 0);
    lv_obj_set_style_bg_opa(hub, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(hub, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_pos(hub, dial / 2 - 6, dial / 2 - 6);

    source_label = lv_label_create(scr);
    lv_label_set_text(source_label, "");
    lv_obj_set_style_text_color(source_label, lv_color_hex(0x9A948F), 0);
    lv_obj_set_style_text_font(source_label, &lv_font_montserrat_20, 0);
    lv_obj_align(source_label, LV_ALIGN_BOTTOM_MID, 0, -72);

#if RAKOS_HAS_BOOT_BUTTON
    lv_obj_t *hint = lv_label_create(scr);
    lv_label_set_text(hint, "BOOT hold 1.5s -> RAKOS");
    lv_obj_set_style_text_color(hint, lv_color_hex(0x666666), 0);
    lv_obj_set_style_text_font(hint, &lv_font_montserrat_20, 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -24);
#endif

    rakos::AppRuntime::addExitButton(scr);
}

#if !defined(RAKOS_BOARD_WAVESHARE_LCD5) && !defined(RAKOS_BOARD_WAVESHARE_LCD5B)
static bool initRtc() {
    if (!rakos::i2cBusProbe(0x51)) {
        return false;
    }

    rakos::I2cLockGuard lock(100);
    if (!lock.locked()) {
        return false;
    }

    if (!rtc.begin(Wire)) {
        return false;
    }

    Serial.printf("[RTC] %s OK\n", rtc.getChipName());
    return true;
}
#endif

void setup() {
    Serial.begin(115200);
    delay(300);
    Serial.println();
    Serial.println("=== ClockApp (RAKOS demo, ota_1) ===");

    BootManager::begin();
    if (!BootManager::isRunningAppSlot()) {
        Serial.println("[WARN] Not running from ota_1");
    }
    initSoftClock();

    if (!rakos::AppRuntime::beginHardware(display, input)) {
        Serial.println("[FATAL] Display init failed");
        for (;;) {
            delay(1000);
        }
    }

    rakos::AppRuntime::buildUiLocked(build_ui);

#if !defined(RAKOS_BOARD_WAVESHARE_LCD5) && !defined(RAKOS_BOARD_WAVESHARE_LCD5B)
    rtc_ok = initRtc();
    if (!rtc_ok) {
        Serial.println("[RTC] Not found — using soft clock");
    }
#else
    Serial.println("[RTC] LCD-5: soft clock (panel owns I2C bus)");
#endif

    struct tm now = {};
    if (getLocalTm(now)) {
        updateClockFace(now);
    }

    last_ui_ms = millis();
    app_ready_ms = last_ui_ms;
    Serial.println("[OK] ClockApp running");
}

void loop() {
    input.update();

    if (rakos::AppRuntime::bootExitRequested(input, app_ready_ms, kExitGraceMs, kExitBootHoldMs)) {
        Serial.println("[APP] Request boot to RAKOS (ota_0)");
        if (BootManager::setNextBootOs()) {
            BootManager::reboot();
        }
    }

    const uint32_t now_ms = millis();
    if ((now_ms - last_ui_ms) >= 200) {
        last_ui_ms = now_ms;
        struct tm t = {};
        if (getLocalTm(t)) {
            updateClockFace(t);
        }
    }

    rakos::AppRuntime::pumpUi(display);
}

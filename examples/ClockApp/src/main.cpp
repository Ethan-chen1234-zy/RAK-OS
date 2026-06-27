/**

 * RAKOS ClockApp — analog + digital watch (ota_1 @ 0x400000)

 *

 * Time priority: NTP (wifi.ini) -> RTC -> soft clock (build time)

 * Tap title: Clock <-> Stopwatch; tap digital in stopwatch: start/pause

 */

#include <Arduino.h>

#include <lvgl.h>

#include <math.h>

#include <time.h>



#include <rakos/app_runtime.h>

#include <rakos/app_time_service.h>

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

static rakos::AppTimeService time_service;



#if !defined(RAKOS_BOARD_WAVESHARE_LCD5) && !defined(RAKOS_BOARD_WAVESHARE_LCD5B)

static SensorRtcHelper rtc;

#endif



static lv_obj_t *title_label = nullptr;

static lv_obj_t *digital_label = nullptr;

static lv_obj_t *date_label = nullptr;

static lv_obj_t *source_label = nullptr;

static lv_obj_t *dial_obj = nullptr;

static lv_obj_t *hour_hand = nullptr;

static lv_obj_t *min_hand = nullptr;

static lv_obj_t *sec_hand = nullptr;



static lv_point_precise_t hour_pts[2];

static lv_point_precise_t min_pts[2];

static lv_point_precise_t sec_pts[2];



static bool rtc_ok = false;

static uint32_t last_ui_ms = 0;

static uint32_t app_ready_ms = 0;



static constexpr uint32_t kExitGraceMs = 3000;

static constexpr uint32_t kExitBootHoldMs = 1500;



static const char *kWeekdays[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};



enum class UiMode : uint8_t { kClock, kStopwatch };



static UiMode ui_mode = UiMode::kClock;

static bool sw_running = false;

static uint32_t sw_elapsed_ms = 0;

static uint32_t sw_start_ms = 0;



static int clockDialSize() {

#if defined(RAKOS_BOARD_WAVESHARE_LCD5) || defined(RAKOS_BOARD_WAVESHARE_LCD5B)

    return 360;

#else

    return 240;

#endif

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



static void syncRtcFromNtp(const struct tm &t) {

    if (!rtc_ok) {

        return;

    }

    rakos::I2cLockGuard lock(100);

    if (!lock.locked()) {

        return;

    }

    rtc.setDateTime((uint16_t)(t.tm_year + 1900),

                    (uint8_t)(t.tm_mon + 1),

                    (uint8_t)t.tm_mday,

                    (uint8_t)t.tm_hour,

                    (uint8_t)t.tm_min,

                    (uint8_t)t.tm_sec);

    Serial.println("[RTC] Synced from NTP");

}

#endif



static bool getLocalTm(struct tm &out) {

    if (time_service.isNtpSynced() && getLocalTime(&out, 0)) {

        return true;

    }

#if !defined(RAKOS_BOARD_WAVESHARE_LCD5) && !defined(RAKOS_BOARD_WAVESHARE_LCD5B)

    if (rtc_ok && readRtcTm(out)) {

        return true;

    }

#endif

    return time_service.getSoftTm(out);

}



static const char *clockSourceLabel() {

    if (time_service.isNtpSynced()) {

        return "NTP";

    }

#if !defined(RAKOS_BOARD_WAVESHARE_LCD5) && !defined(RAKOS_BOARD_WAVESHARE_LCD5B)

    if (rtc_ok) {

        return "RTC";

    }

#endif

    return "Soft clock";

}



static void updateSourceLabel() {

    if (!source_label) {

        return;

    }

    char buf[56];

    snprintf(buf,

             sizeof(buf),

             "%s  |  %s",

             clockSourceLabel(),

             time_service.networkLabel());

    lv_label_set_text(source_label, buf);

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

        lv_obj_clear_flag(date_label, LV_OBJ_FLAG_HIDDEN);

    }

    updateSourceLabel();

}



static uint32_t stopwatchNowMs() {

    if (!sw_running) {

        return sw_elapsed_ms;

    }

    return sw_elapsed_ms + (millis() - sw_start_ms);

}



static void updateStopwatchFace() {

    const uint32_t ms = stopwatchNowMs();

    const uint32_t total_s = ms / 1000U;

    const uint32_t min = total_s / 60U;

    const uint32_t sec = total_s % 60U;

    const uint32_t centi = (ms % 1000U) / 10U;



    if (digital_label) {

        lv_label_set_text_fmt(digital_label, "%02lu:%02lu.%02lu", (unsigned long)min, (unsigned long)sec,

                              (unsigned long)centi);

    }

    if (date_label) {

        lv_label_set_text(date_label, sw_running ? "Tap time to pause" : "Tap time to start");

    }

    if (source_label) {

        lv_label_set_text(source_label, "Stopwatch");

    }

}



static void applyUiMode() {

    const bool clock = ui_mode == UiMode::kClock;

    if (title_label) {

        lv_label_set_text(title_label, clock ? "Clock" : "Stopwatch");

    }

    if (dial_obj) {

        if (clock) {

            lv_obj_clear_flag(dial_obj, LV_OBJ_FLAG_HIDDEN);

        } else {

            lv_obj_add_flag(dial_obj, LV_OBJ_FLAG_HIDDEN);

        }

    }

    if (clock) {

        struct tm t = {};

        if (getLocalTm(t)) {

            updateClockFace(t);

        }

    } else {

        updateStopwatchFace();

    }

}



static void on_title_clicked(lv_event_t *e) {

    (void)e;

    ui_mode = (ui_mode == UiMode::kClock) ? UiMode::kStopwatch : UiMode::kClock;

    if (ui_mode == UiMode::kStopwatch) {

        sw_running = false;

        sw_elapsed_ms = 0;

        sw_start_ms = millis();

    }

    applyUiMode();

}



static void on_digital_clicked(lv_event_t *e) {

    (void)e;

    if (ui_mode != UiMode::kStopwatch) {

        return;

    }

    if (sw_running) {

        sw_elapsed_ms = stopwatchNowMs();

        sw_running = false;

    } else {

        sw_start_ms = millis();

        sw_running = true;

    }

    updateStopwatchFace();

}



static void build_ui() {

    lv_obj_t *scr = lv_screen_active();

    lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), 0);

    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);



    const int dial = clockDialSize();



    title_label = lv_label_create(scr);

    lv_label_set_text(title_label, "Clock");

    lv_obj_set_style_text_color(title_label, lv_color_hex(0xFF7F1F), 0);

    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_24, 0);

    lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 24);

    lv_obj_add_flag(title_label, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_add_event_cb(title_label, on_title_clicked, LV_EVENT_CLICKED, nullptr);



    digital_label = lv_label_create(scr);

    lv_label_set_text(digital_label, "--:--:--");

    lv_obj_set_style_text_color(digital_label, lv_color_hex(0xFFFAF5), 0);

    lv_obj_set_style_text_font(digital_label, &lv_font_montserrat_24, 0);

    lv_obj_align(digital_label, LV_ALIGN_TOP_MID, 0, 64);

    lv_obj_add_flag(digital_label, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_add_event_cb(digital_label, on_digital_clicked, LV_EVENT_CLICKED, nullptr);



    date_label = lv_label_create(scr);

    lv_label_set_text(date_label, "---");

    lv_obj_set_style_text_color(date_label, lv_color_hex(0xB0AAA4), 0);

    lv_obj_set_style_text_font(date_label, &lv_font_montserrat_20, 0);

    lv_obj_align(date_label, LV_ALIGN_TOP_MID, 0, 104);



    dial_obj = lv_obj_create(scr);

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

    lv_label_set_text(hint, "Title: stopwatch  |  BOOT hold -> RAKOS");

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



    if (!rakos::AppRuntime::beginHardware(display, input)) {

        Serial.println("[FATAL] Display init failed");

        for (;;) {

            delay(1000);

        }

    }



#if !defined(RAKOS_BOARD_WAVESHARE_LCD5) && !defined(RAKOS_BOARD_WAVESHARE_LCD5B)

    rtc_ok = initRtc();

    if (!rtc_ok) {

        Serial.println("[RTC] Not found — using NTP or soft clock");

    }

    time_service.setRtcSyncCallback(syncRtcFromNtp);

#else

    Serial.println("[RTC] LCD-5: NTP or soft clock (panel owns I2C bus)");

#endif



    time_service.begin();



    rakos::AppRuntime::buildUiLocked(build_ui);



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

    time_service.update();



    if (rakos::AppRuntime::bootExitRequested(input, app_ready_ms, kExitGraceMs, kExitBootHoldMs)) {

        Serial.println("[APP] Request boot to RAKOS (ota_0)");

        if (BootManager::setNextBootOs()) {

            BootManager::reboot();

        }

    }



    const uint32_t now_ms = millis();

    if ((now_ms - last_ui_ms) >= 100) {

        last_ui_ms = now_ms;

        if (ui_mode == UiMode::kClock) {

            struct tm t = {};

            if (getLocalTm(t)) {

                updateClockFace(t);

            }

        } else {

            updateStopwatchFace();

        }

    }



    rakos::AppRuntime::pumpUi(display, app_ready_ms);

}



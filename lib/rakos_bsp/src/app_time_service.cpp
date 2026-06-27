#include <rakos/app_time_service.h>
#include <rakos/storage_service.h>
#include <rakos/wifi_sd_config.h>

#include <WiFi.h>
#include <time.h>
#include <cstring>

namespace {

constexpr int kGmtOffsetSec = 8 * 3600;
constexpr int kDaylightOffsetSec = 0;

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

}  // namespace

namespace rakos {

bool AppTimeService::begin() {
    initSoftClockFromBuild();

    storage_ready_ = storage_.init();
    if (!storage_ready_) {
        Serial.println("[TIME] SD mount skipped — soft clock only");
    }

    WifiSdConfig wifi;
    if (loadWifiFromSd(wifi)) {
        strncpy(wifi_ssid_, wifi.ssid, sizeof(wifi_ssid_) - 1);
        strncpy(wifi_pass_, wifi.pass, sizeof(wifi_pass_) - 1);
        wifi_cfg_loaded_ = true;
        startWifiConnect();
    } else {
        Serial.println("[TIME] No wifi.ini — NTP unavailable");
    }

    return true;
}

void AppTimeService::initSoftClockFromBuild() {
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
    soft_epoch_ = mktime(&t);
    soft_millis_base_ = millis();
}

void AppTimeService::startWifiConnect() {
    if (!wifi_cfg_loaded_ || wifi_ssid_[0] == '\0') {
        wifi_state_ = AppWifiState::kFailed;
        return;
    }

    WiFi.disconnect(true);
    WiFi.mode(WIFI_STA);
    WiFi.begin(wifi_ssid_, wifi_pass_);
    wifi_state_ = AppWifiState::kConnecting;
    connect_started_ms_ = millis();
    ntp_started_ = false;
    ntp_synced_ = false;
    Serial.printf("[TIME] WiFi connecting to \"%s\"...\n", wifi_ssid_);
}

void AppTimeService::ensureNtp() {
    if (ntp_started_) {
        return;
    }
    ntp_started_ = true;
    configTime(kGmtOffsetSec, kDaylightOffsetSec, "ntp.aliyun.com", "pool.ntp.org");
    Serial.println("[TIME] NTP sync started (UTC+8)");
}

void AppTimeService::pollNtp() {
    if (ntp_synced_) {
        return;
    }
    struct tm ti;
    if (!getLocalTime(&ti, 200)) {
        return;
    }
    ntp_synced_ = true;
    Serial.printf("[TIME] NTP synced: %04d-%02d-%02d %02d:%02d:%02d\n",
                  ti.tm_year + 1900,
                  ti.tm_mon + 1,
                  ti.tm_mday,
                  ti.tm_hour,
                  ti.tm_min,
                  ti.tm_sec);
    if (rtc_sync_fn_) {
        rtc_sync_fn_(ti);
    }
}

void AppTimeService::update() {
    if (wifi_state_ == AppWifiState::kConnecting) {
        if (WiFi.status() == WL_CONNECTED) {
            wifi_state_ = AppWifiState::kConnected;
            Serial.printf("[TIME] WiFi connected, IP=%s\n", WiFi.localIP().toString().c_str());
            ensureNtp();
        } else if ((millis() - connect_started_ms_) > 20000U) {
            wifi_state_ = AppWifiState::kFailed;
            Serial.println("[TIME] WiFi connect timeout");
        }
        return;
    }

    if (wifi_state_ == AppWifiState::kConnected) {
        ensureNtp();
        pollNtp();
    }
}

bool AppTimeService::getSoftTm(struct tm &out) const {
    const time_t now = soft_epoch_ + (time_t)((millis() - soft_millis_base_) / 1000U);
    localtime_r(&now, &out);
    return true;
}

AppTimeSource AppTimeService::activeSource() const {
    if (ntp_synced_) {
        return AppTimeSource::kNtp;
    }
    return AppTimeSource::kSoft;
}

const char *AppTimeService::sourceLabel() const {
    switch (activeSource()) {
        case AppTimeSource::kNtp:
            return "NTP";
        case AppTimeSource::kRtc:
            return "RTC";
        default:
            return "Soft clock";
    }
}

const char *AppTimeService::networkLabel() const {
    if (!wifi_cfg_loaded_) {
        return "No wifi.ini";
    }
    switch (wifi_state_) {
        case AppWifiState::kConnecting:
            return "WiFi connecting...";
        case AppWifiState::kConnected:
            return ntp_synced_ ? "WiFi + NTP" : "WiFi (NTP...)";
        case AppWifiState::kFailed:
            return "WiFi failed";
        default:
            return "WiFi off";
    }
}

}  // namespace rakos

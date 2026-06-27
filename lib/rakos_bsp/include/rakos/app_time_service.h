#pragma once

#include <Arduino.h>
#include <rakos/storage_service.h>
#include <functional>
#include <time.h>

namespace rakos {

enum class AppTimeSource : uint8_t {
    kSoft,
    kRtc,
    kNtp,
};

enum class AppWifiState : uint8_t {
    kOff,
    kConnecting,
    kConnected,
    kFailed,
};

/** WiFi (SD wifi.ini) + NTP for user apps; soft-clock fallback when offline. */
class AppTimeService {
public:
    AppTimeService() = default;
    ~AppTimeService() = default;

    bool begin();
    void update();

    bool isNtpSynced() const { return ntp_synced_; }
    AppWifiState wifiState() const { return wifi_state_; }
    AppTimeSource activeSource() const;

    bool getSoftTm(struct tm &out) const;
    const char *sourceLabel() const;
    const char *networkLabel() const;

    using RtcSyncFn = std::function<void(const struct tm &)>;
    void setRtcSyncCallback(RtcSyncFn fn) { rtc_sync_fn_ = std::move(fn); }

private:
    StorageService storage_;
    bool storage_ready_ = false;
    AppWifiState wifi_state_ = AppWifiState::kOff;
    bool ntp_started_ = false;
    bool ntp_synced_ = false;
    bool wifi_cfg_loaded_ = false;
    char wifi_ssid_[33] = {};
    char wifi_pass_[65] = {};
    time_t soft_epoch_ = 0;
    uint32_t soft_millis_base_ = 0;
    uint32_t connect_started_ms_ = 0;
    RtcSyncFn rtc_sync_fn_;

    void initSoftClockFromBuild();
    void startWifiConnect();
    void ensureNtp();
    void pollNtp();
};

}  // namespace rakos

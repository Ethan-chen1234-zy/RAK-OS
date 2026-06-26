#pragma once

#include <Arduino.h>

struct RadioConfig {
    bool wifi_enabled = false;
    char wifi_ssid[33] = {};
    char wifi_pass[65] = {};
    bool ble_enabled = false;
    char ble_name[21] = "RAKOS";
};

enum class WifiLinkState : uint8_t {
    kOff,
    kScanning,
    kConnecting,
    kConnected,
    kFailed,
};

class RadioService {
public:
    bool begin();
    void update();

    const RadioConfig &config() const { return cfg_; }
    void setConfig(const RadioConfig &cfg);
    void saveConfig();

    void setWifiEnabled(bool on);
    void setWifiCredentials(const char *ssid, const char *pass);
    void setBleEnabled(bool on);
    void setBleName(const char *name);

    /** Apply current config (connect WiFi / start BLE). Safe from UI thread. */
    void apply();

    /** If wifi.ini on SD exists, load it and connect (overrides NVS WiFi fields). */
    bool loadAndApplySdWifi();

    /** Retry SD wifi.ini read (e.g. after SD mount stabilizes). */
    bool tryApplyWifiFromSd();

    void notifySdUnmounted();

    WifiLinkState wifiState() const { return wifi_state_; }
    bool bleActive() const { return ble_active_; }
    bool isTimeSynced() const { return time_synced_; }
    String wifiStatusText() const;
    String bleStatusText() const;
    String ipAddress() const;
    /** Local time "HH:MM" after NTP sync; empty otherwise. */
    String localTimeText() const;

private:
    RadioConfig cfg_;
    WifiLinkState wifi_state_ = WifiLinkState::kOff;
    bool ble_active_ = false;
    bool ntp_started_ = false;
    bool time_synced_ = false;
    bool wifi_from_sd_applied_ = false;
    uint32_t connect_started_ms_ = 0;
    uint32_t scan_started_ms_ = 0;
    bool loaded_ = false;

    void loadConfig();
    void startWifiConnect();
    void stopWifi();
    void startBle();
    void stopBle();
    void ensureNtp();
    void pollNtp();
};

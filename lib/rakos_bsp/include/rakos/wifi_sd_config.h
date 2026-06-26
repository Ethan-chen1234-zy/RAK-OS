#pragma once

#include <Arduino.h>

struct WifiSdConfig {
    char ssid[33] = {};
    char pass[65] = {};
    bool valid = false;
};

/** Load `ssid` / `password` from SD (default: /sdcard/wifi.ini). */
bool loadWifiFromSd(WifiSdConfig &out);

/** Write credentials to SD (no-op if SD not mounted). */
bool saveWifiToSd(const char *ssid, const char *pass);

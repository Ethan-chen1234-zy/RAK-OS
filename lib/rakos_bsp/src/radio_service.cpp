#include <rakos/radio_service.h>
#include <rakos/wifi_sd_config.h>
#include <Preferences.h>
#include <WiFi.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <time.h>
#include <cstring>

static Preferences g_radio_prefs;
static BLEServer *g_ble_server = nullptr;

static constexpr int kGmtOffsetSec = 8 * 3600;
static constexpr int kDaylightOffsetSec = 0;

// Minimal GATT service so scanners see a connectable peripheral (not just a tag).
static constexpr const char *kRakosServiceUuid = "a1b2c3d4-e5f6-7890-abcd-ef1234567890";
static constexpr const char *kRakosCharUuid = "b1c2d3e4-f5a6-7890-bcde-f12345678901";

static void trimCopy(char *dst, size_t dst_len, const char *src) {
    if (!dst || dst_len == 0) {
        return;
    }
    if (!src) {
        dst[0] = '\0';
        return;
    }
    strncpy(dst, src, dst_len - 1);
    dst[dst_len - 1] = '\0';
}

bool RadioService::begin() {
    loadConfig();
    loaded_ = true;
    apply();
    return true;
}

bool RadioService::loadAndApplySdWifi() {
    return tryApplyWifiFromSd();
}

bool RadioService::tryApplyWifiFromSd() {
    WifiSdConfig sd;
    if (!loadWifiFromSd(sd)) {
        return false;
    }

    const bool same = wifi_from_sd_applied_ && strncmp(cfg_.wifi_ssid, sd.ssid, sizeof(cfg_.wifi_ssid)) == 0 &&
                      strncmp(cfg_.wifi_pass, sd.pass, sizeof(cfg_.wifi_pass)) == 0;
    if (same && (wifi_state_ == WifiLinkState::kConnected || wifi_state_ == WifiLinkState::kConnecting ||
                 wifi_state_ == WifiLinkState::kScanning)) {
        return true;
    }

    wifi_from_sd_applied_ = true;
    setWifiCredentials(sd.ssid, sd.pass);
    setWifiEnabled(true);
    time_synced_ = false;
    ntp_started_ = false;
    apply();
    return true;
}

void RadioService::notifySdUnmounted() {
    if (wifi_state_ != WifiLinkState::kConnected) {
        wifi_from_sd_applied_ = false;
    }
}

void RadioService::loadConfig() {
    if (!g_radio_prefs.begin("rakos_radio", false)) {
        return;
    }
    cfg_.wifi_enabled = g_radio_prefs.getBool("wifi_en", false);
    cfg_.ble_enabled = g_radio_prefs.getBool("ble_en", false);
    String ssid = g_radio_prefs.getString("wifi_ssid", "");
    String pass = g_radio_prefs.getString("wifi_pass", "");
    String name = g_radio_prefs.getString("ble_name", "RAKOS");
    trimCopy(cfg_.wifi_ssid, sizeof(cfg_.wifi_ssid), ssid.c_str());
    trimCopy(cfg_.wifi_pass, sizeof(cfg_.wifi_pass), pass.c_str());
    trimCopy(cfg_.ble_name, sizeof(cfg_.ble_name), name.c_str());
    if (cfg_.ble_name[0] == '\0') {
        strncpy(cfg_.ble_name, "RAKOS", sizeof(cfg_.ble_name) - 1);
    }
}

void RadioService::saveConfig() {
    if (!g_radio_prefs.begin("rakos_radio", false)) {
        return;
    }
    g_radio_prefs.putBool("wifi_en", cfg_.wifi_enabled);
    g_radio_prefs.putBool("ble_en", cfg_.ble_enabled);
    g_radio_prefs.putString("wifi_ssid", cfg_.wifi_ssid);
    g_radio_prefs.putString("wifi_pass", cfg_.wifi_pass);
    g_radio_prefs.putString("ble_name", cfg_.ble_name);
}

void RadioService::setConfig(const RadioConfig &cfg) {
    cfg_ = cfg;
}

void RadioService::setWifiEnabled(bool on) {
    cfg_.wifi_enabled = on;
}

void RadioService::setWifiCredentials(const char *ssid, const char *pass) {
    trimCopy(cfg_.wifi_ssid, sizeof(cfg_.wifi_ssid), ssid);
    trimCopy(cfg_.wifi_pass, sizeof(cfg_.wifi_pass), pass);
}

void RadioService::setBleEnabled(bool on) {
    cfg_.ble_enabled = on;
}

void RadioService::setBleName(const char *name) {
    trimCopy(cfg_.ble_name, sizeof(cfg_.ble_name), name);
    if (cfg_.ble_name[0] == '\0') {
        strncpy(cfg_.ble_name, "RAKOS", sizeof(cfg_.ble_name) - 1);
    }
}

void RadioService::stopWifi() {
    WiFi.disconnect(true);
    WiFi.scanDelete();
    WiFi.mode(WIFI_OFF);
    wifi_state_ = WifiLinkState::kOff;
    time_synced_ = false;
    ntp_started_ = false;
}

void RadioService::startWifiConnect() {
    if (cfg_.wifi_ssid[0] == '\0') {
        wifi_state_ = WifiLinkState::kFailed;
        Serial.println("[RADIO] WiFi SSID empty");
        return;
    }

    stopWifi();
    wifi_state_ = WifiLinkState::kScanning;
    scan_started_ms_ = millis();

    WiFi.mode(WIFI_STA);
    WiFi.disconnect(true, true);
    delay(50);
    Serial.printf("[RADIO] Scanning for \"%s\"...\n", cfg_.wifi_ssid);
    WiFi.scanNetworks(true, true);
}

void RadioService::stopBle() {
    if (ble_active_) {
        BLEDevice::deinit(true);
        g_ble_server = nullptr;
        ble_active_ = false;
        Serial.println("[RADIO] BLE stopped");
    }
}

void RadioService::startBle() {
    stopBle();
    if (cfg_.ble_name[0] == '\0') {
        ble_active_ = false;
        return;
    }

    if (!BLEDevice::init(cfg_.ble_name)) {
        Serial.println("[RADIO] BLE init failed");
        ble_active_ = false;
        return;
    }

    g_ble_server = BLEDevice::createServer();
    g_ble_server->advertiseOnDisconnect(true);

    BLEService *service = g_ble_server->createService(kRakosServiceUuid);
    BLECharacteristic *info = service->createCharacteristic(
        kRakosCharUuid,
        BLECharacteristic::PROPERTY_READ);
    info->setValue("RAKOS");
    service->start();

    BLEAdvertising *adv = BLEDevice::getAdvertising();
    adv->addServiceUUID(kRakosServiceUuid);
    adv->setScanResponse(true);
    adv->setName(cfg_.ble_name);
    adv->setMinPreferred(0x06);
    adv->setMaxPreferred(0x12);
    BLEDevice::startAdvertising();

    ble_active_ = true;
    Serial.printf("[RADIO] BLE advertising as \"%s\"\n", cfg_.ble_name);
}

void RadioService::apply() {
    if (!loaded_) {
        loadConfig();
        loaded_ = true;
    }

    if (cfg_.wifi_enabled) {
        startWifiConnect();
    } else {
        stopWifi();
    }

    if (cfg_.ble_enabled) {
        startBle();
    } else {
        stopBle();
    }

    saveConfig();
}

void RadioService::ensureNtp() {
    if (ntp_started_) {
        return;
    }
    ntp_started_ = true;
    configTime(kGmtOffsetSec, kDaylightOffsetSec, "ntp.aliyun.com", "pool.ntp.org");
    Serial.println("[RADIO] NTP sync started (UTC+8)");
}

void RadioService::pollNtp() {
    if (time_synced_) {
        return;
    }
    struct tm ti;
    if (getLocalTime(&ti, 200)) {
        time_synced_ = true;
        Serial.printf("[RADIO] Time synced: %04d-%02d-%02d %02d:%02d:%02d\n",
                      ti.tm_year + 1900,
                      ti.tm_mon + 1,
                      ti.tm_mday,
                      ti.tm_hour,
                      ti.tm_min,
                      ti.tm_sec);
    }
}

void RadioService::update() {
    if (wifi_state_ == WifiLinkState::kScanning) {
        const int n = WiFi.scanComplete();
        if (n == WIFI_SCAN_RUNNING) {
            if ((millis() - scan_started_ms_) > 12000U) {
                WiFi.scanDelete();
                wifi_state_ = WifiLinkState::kFailed;
                Serial.println("[RADIO] WiFi scan timeout");
            }
            return;
        }
        if (n == WIFI_SCAN_FAILED) {
            wifi_state_ = WifiLinkState::kFailed;
            Serial.println("[RADIO] WiFi scan failed");
            return;
        }

        bool seen = false;
        Serial.printf("[RADIO] Scan: %d network(s)\n", n);
        for (int i = 0; i < n; ++i) {
            Serial.printf("  %s  %ddBm\n", WiFi.SSID(i).c_str(), WiFi.RSSI(i));
            if (WiFi.SSID(i) == cfg_.wifi_ssid) {
                seen = true;
            }
        }
        if (!seen) {
            Serial.printf("[RADIO] \"%s\" not in scan — connecting anyway\n", cfg_.wifi_ssid);
        }

        WiFi.scanDelete();
        wifi_state_ = WifiLinkState::kConnecting;
        connect_started_ms_ = millis();
        WiFi.begin(cfg_.wifi_ssid, cfg_.wifi_pass);
        Serial.printf("[RADIO] Connecting to \"%s\"...\n", cfg_.wifi_ssid);
        return;
    }

    if (wifi_state_ != WifiLinkState::kConnecting && wifi_state_ != WifiLinkState::kConnected) {
        return;
    }

    const wl_status_t st = WiFi.status();
    if (st == WL_CONNECTED) {
        if (wifi_state_ != WifiLinkState::kConnected) {
            wifi_state_ = WifiLinkState::kConnected;
            Serial.printf("[RADIO] WiFi connected, IP=%s\n", WiFi.localIP().toString().c_str());
        }
        ensureNtp();
        pollNtp();
        return;
    }

    if (wifi_state_ == WifiLinkState::kConnecting && (millis() - connect_started_ms_) > 20000U) {
        wifi_state_ = WifiLinkState::kFailed;
        Serial.println("[RADIO] WiFi connect timeout");
    }
}

String RadioService::ipAddress() const {
    if (wifi_state_ == WifiLinkState::kConnected) {
        return WiFi.localIP().toString();
    }
    return String();
}

String RadioService::localTimeText() const {
    if (!time_synced_) {
        return String();
    }
    struct tm ti;
    if (!getLocalTime(&ti)) {
        return String();
    }
    char buf[8];
    snprintf(buf, sizeof(buf), "%02d:%02d", ti.tm_hour, ti.tm_min);
    return String(buf);
}

String RadioService::wifiStatusText() const {
    if (!cfg_.wifi_enabled) {
        return "WiFi off";
    }
    switch (wifi_state_) {
        case WifiLinkState::kScanning:
            return "Scanning...";
        case WifiLinkState::kConnecting:
            return "Connecting...";
        case WifiLinkState::kConnected: {
            String s = "Connected " + ipAddress();
            if (time_synced_) {
                s += "  ";
                s += localTimeText();
            }
            return s;
        }
        case WifiLinkState::kFailed:
            return "Failed (check SSID/password)";
        default:
            return "Off";
    }
}

String RadioService::bleStatusText() const {
    if (!cfg_.ble_enabled) {
        return "BLE off";
    }
    if (ble_active_) {
        return String("Advertising: ") + cfg_.ble_name;
    }
    return "BLE start failed";
}

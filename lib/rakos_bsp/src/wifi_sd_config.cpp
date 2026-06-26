#include <rakos/wifi_sd_config.h>
#include <rakos/pin_config.h>
#include <rakos/sd_fs.h>
#include <FS.h>
#include <cstring>
#include <strings.h>

namespace {

constexpr const char *kWifiPaths[] = {
    "/wifi.ini",
    "/sdcard/wifi.ini",
    "/sdcard//wifi.ini",
};

void trimInPlace(char *s) {
    if (!s) {
        return;
    }
    char *start = s;
    while (*start == ' ' || *start == '\t' || *start == '\r') {
        ++start;
    }
    if (start != s) {
        memmove(s, start, strlen(start) + 1);
    }
    size_t len = strlen(s);
    while (len > 0 && (s[len - 1] == ' ' || s[len - 1] == '\t' || s[len - 1] == '\r')) {
        s[--len] = '\0';
    }
}

bool parseLine(const char *line, WifiSdConfig &cfg) {
    if (!line || line[0] == '#' || line[0] == '\0') {
        return false;
    }
    const char *eq = strchr(line, '=');
    if (!eq || eq == line) {
        return false;
    }

    char key[16];
    const size_t key_len = static_cast<size_t>(eq - line);
    if (key_len >= sizeof(key)) {
        return false;
    }
    memcpy(key, line, key_len);
    key[key_len] = '\0';
    trimInPlace(key);

    char val[96];
    strncpy(val, eq + 1, sizeof(val) - 1);
    val[sizeof(val) - 1] = '\0';
    trimInPlace(val);
    if (val[0] == '\0') {
        return false;
    }

    if (strcasecmp(key, "ssid") == 0) {
        strncpy(cfg.ssid, val, sizeof(cfg.ssid) - 1);
        cfg.ssid[sizeof(cfg.ssid) - 1] = '\0';
        return true;
    }
    if (strcasecmp(key, "password") == 0 || strcasecmp(key, "pass") == 0) {
        strncpy(cfg.pass, val, sizeof(cfg.pass) - 1);
        cfg.pass[sizeof(cfg.pass) - 1] = '\0';
        return true;
    }
    return false;
}

File openWifiFile(const char *mode) {
    fs::FS &fs = rakos::sdFs();
    for (const char *path : kWifiPaths) {
        File f = fs.open(path, mode);
        if (f) {
            return f;
        }
    }
    return File();
}

} // namespace

bool loadWifiFromSd(WifiSdConfig &out) {
    out = WifiSdConfig{};

    File f = openWifiFile(FILE_READ);
    if (!f || f.isDirectory()) {
        Serial.println("[WIFI] No wifi.ini on SD — copy to TF card root (see sd/wifi.ini.example)");
        return false;
    }

    while (f.available()) {
        String line = f.readStringUntil('\n');
        line.trim();
        if (line.isEmpty() || line.startsWith("#")) {
            continue;
        }
        parseLine(line.c_str(), out);
    }
    f.close();

    out.valid = out.ssid[0] != '\0';
    if (out.valid) {
        Serial.printf("[WIFI] SD config: ssid=\"%s\"\n", out.ssid);
    }
    return out.valid;
}

bool saveWifiToSd(const char *ssid, const char *pass) {
    if (!ssid || !ssid[0]) {
        return false;
    }
    if (!pass) {
        pass = "";
    }

    fs::FS &fs = rakos::sdFs();
    File probe = fs.open("/sdcard", FILE_READ);
    if (!probe) {
        probe = fs.open("/", FILE_READ);
    }
    if (!probe) {
        Serial.println("[WIFI] SD not mounted, skip save wifi.ini");
        return false;
    }
    probe.close();

    const char *path = "/wifi.ini";
    File f = fs.open(path, FILE_WRITE);
    if (!f) {
        path = "/sdcard/wifi.ini";
        f = fs.open(path, FILE_WRITE);
    }
    if (!f) {
        Serial.println("[WIFI] Cannot write wifi.ini on SD");
        return false;
    }

    f.printf("# RAKOS WiFi — edit on PC or via Setup -> Save\n");
    f.printf("ssid=%s\n", ssid);
    f.printf("password=%s\n", pass);
    f.close();
    Serial.printf("[WIFI] Saved %s\n", path);
    return true;
}

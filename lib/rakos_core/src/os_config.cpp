#include <rakos/os_config.h>
#include <Preferences.h>
#include <esp_partition.h>

static bool isValidAppSubtype(uint8_t subtype) {
    return subtype == ESP_PARTITION_SUBTYPE_APP_OTA_0 || subtype == ESP_PARTITION_SUBTYPE_APP_OTA_1;
}

bool OsConfig::begin() {
    Preferences prefs;
    prefs.begin("rakos_cfg", false);
    cfg_.mode = static_cast<OsMode>(prefs.getUChar("mode", static_cast<uint8_t>(OsMode::kManaged)));
    cfg_.default_boot_subtype = prefs.getUChar("def_boot", ESP_PARTITION_SUBTYPE_APP_OTA_0);
    if (!isValidAppSubtype(cfg_.default_boot_subtype)) {
        cfg_.default_boot_subtype = ESP_PARTITION_SUBTYPE_APP_OTA_0;
    }
    cfg_.boot_counter = prefs.getUInt("boots", 0);
    cfg_.boot_counter += 1;
    prefs.putUInt("boots", cfg_.boot_counter);
    return true;
}

void OsConfig::setMode(OsMode mode) {
    cfg_.mode = mode;
}

void OsConfig::setDefaultBootSubtype(uint8_t subtype) {
    if (isValidAppSubtype(subtype)) {
        cfg_.default_boot_subtype = subtype;
    }
}

void OsConfig::save() {
    Preferences prefs;
    prefs.begin("rakos_cfg", false);
    prefs.putUChar("mode", static_cast<uint8_t>(cfg_.mode));
    prefs.putUChar("def_boot", cfg_.default_boot_subtype);
}

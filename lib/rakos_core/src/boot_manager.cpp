#include <rakos/boot_manager.h>
#include <esp_ota_ops.h>

void BootManager::begin() {
    logSummary();
}

PartitionSummary BootManager::getSummary() {
    PartitionSummary s;
    s.running = esp_ota_get_running_partition();
    s.boot = esp_ota_get_boot_partition();
    s.ota0 = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, nullptr);
    s.ota1 = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_1, nullptr);
    return s;
}

bool BootManager::isRunningOsSlot() {
    const esp_partition_t *running = esp_ota_get_running_partition();
    return running && running->subtype == ESP_PARTITION_SUBTYPE_APP_OTA_0;
}

bool BootManager::isRunningAppSlot() {
    const esp_partition_t *running = esp_ota_get_running_partition();
    return running && running->subtype == ESP_PARTITION_SUBTYPE_APP_OTA_1;
}

bool BootManager::setNextBootOs() {
    const esp_partition_t *target =
        esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, nullptr);
    if (!target) {
        return false;
    }
    return esp_ota_set_boot_partition(target) == ESP_OK;
}

bool BootManager::setNextBootApp() {
    const esp_partition_t *target =
        esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_1, nullptr);
    if (!target) {
        return false;
    }
    return esp_ota_set_boot_partition(target) == ESP_OK;
}

bool BootManager::ota1HasFirmware() {
    const esp_partition_t *part =
        esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_1, nullptr);
    if (!part) {
        return false;
    }
    uint8_t magic = 0;
    if (esp_partition_read(part, 0, &magic, 1) != ESP_OK) {
        return false;
    }
    return magic == 0xE9;
}

void BootManager::reboot() {
    delay(80);
    esp_restart();
}

int BootManager::getOtaState(const esp_partition_t *part) {
    if (!part) {
        return -1;
    }
    esp_ota_img_states_t state = ESP_OTA_IMG_UNDEFINED;
    if (esp_ota_get_state_partition(part, &state) != ESP_OK) {
        return -1;
    }
    return static_cast<int>(state);
}

void BootManager::logSummary() {
    const PartitionSummary s = getSummary();
    auto logPart = [](const char *label, const esp_partition_t *p) {
        if (!p) {
            Serial.printf("[BOOT] %s: null\n", label);
            return;
        }
        Serial.printf("[BOOT] %s label=%s addr=0x%08X size=0x%08X state=%d\n",
                      label,
                      p->label,
                      (unsigned)p->address,
                      (unsigned)p->size,
                      BootManager::getOtaState(p));
    };
    logPart("running", s.running);
    logPart("boot", s.boot);
    logPart("ota_0", s.ota0);
    logPart("ota_1", s.ota1);
}

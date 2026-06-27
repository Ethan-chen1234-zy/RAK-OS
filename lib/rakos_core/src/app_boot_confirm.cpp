#include <rakos/app_boot_confirm.h>
#include <rakos/boot_manager.h>

#include <esp_ota_ops.h>

namespace rakos {

void AppBootConfirm::tick(uint32_t app_ready_ms, uint32_t grace_ms) {
    static bool confirmed = false;
    if (confirmed || !BootManager::isRunningAppSlot()) {
        return;
    }
    if ((millis() - app_ready_ms) < grace_ms) {
        return;
    }

    const esp_partition_t *running = esp_ota_get_running_partition();
    if (!running) {
        return;
    }

    esp_ota_img_states_t state = ESP_OTA_IMG_UNDEFINED;
    if (esp_ota_get_state_partition(running, &state) != ESP_OK) {
        return;
    }
    if (state != ESP_OTA_IMG_PENDING_VERIFY) {
        confirmed = true;
        return;
    }

    if (esp_ota_mark_app_valid_cancel_rollback() == ESP_OK) {
        Serial.println("[APP] Boot confirmed — rollback cancelled");
        confirmed = true;
    }
}

}  // namespace rakos

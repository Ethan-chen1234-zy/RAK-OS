#include <rakos/app_installer.h>
#include <rakos/pin_config.h>
#include <rakos/sd_fs.h>

#include <FS.h>
#include <Preferences.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>
#include <esp_rom_crc.h>

namespace {

constexpr const char *kPrefsNs = "rakos_install";
constexpr const char *kKeyPath = "path";
constexpr const char *kKeyCrc = "crc";
constexpr const char *kKeySize = "size";

File openSdAppBin(const String &path) {
    fs::FS &fs = rakos::sdFs();
    File f = fs.open(path.c_str(), FILE_READ);
    if (f) {
        return f;
    }
    if (!path.startsWith(SD_MOUNT_POINT)) {
        const String alt = String(SD_MOUNT_POINT) + (path.startsWith("/") ? path : String("/") + path);
        f = fs.open(alt.c_str(), FILE_READ);
        if (f) {
            return f;
        }
    }
    if (path.startsWith(SD_MOUNT_POINT)) {
        const String alt = path.substring(strlen(SD_MOUNT_POINT));
        if (alt.length() == 0 || alt == "/") {
            f = fs.open("/", FILE_READ);
        } else {
            f = fs.open(alt.c_str(), FILE_READ);
        }
    }
    if (!f && path.startsWith("/") && String(SD_FS_ROOT) == "/") {
        f = fs.open(path.c_str(), FILE_READ);
    }
    return f;
}

bool readStoredInstall(String &path, uint32_t &crc, size_t &size) {
    Preferences prefs;
    if (!prefs.begin(kPrefsNs, true)) {
        return false;
    }
    path = prefs.getString(kKeyPath, "");
    crc = static_cast<uint32_t>(prefs.getUInt(kKeyCrc, 0));
    size = static_cast<size_t>(prefs.getUInt(kKeySize, 0));
    return !path.isEmpty() && size > 0;
}

void storeInstall(const char *path, uint32_t crc, size_t size) {
    Preferences prefs;
    if (!prefs.begin(kPrefsNs, false)) {
        return;
    }
    prefs.putString(kKeyPath, path);
    prefs.putUInt(kKeyCrc, crc);
    prefs.putUInt(kKeySize, static_cast<uint32_t>(size));
}

uint32_t crc32File(File &f) {
    uint32_t crc = 0;
    uint8_t buf[4096];
    while (f.available()) {
        const size_t n = f.read(buf, sizeof(buf));
        if (n == 0) {
            break;
        }
        crc = esp_rom_crc32_le(crc, buf, n);
    }
    return crc;
}

bool validateImageHeader(File &f) {
    uint8_t magic = 0;
    if (!f.seek(0) || f.read(&magic, 1) != 1) {
        return false;
    }
    return magic == 0xE9;
}

}  // namespace

namespace rakos {

bool AppInstaller::isSameAsInstalled(const char *path, uint32_t &out_crc, size_t &out_size) {
    out_crc = 0;
    out_size = 0;
    if (!path || !path[0]) {
        return false;
    }

    String stored_path;
    uint32_t stored_crc = 0;
    size_t stored_size = 0;
    if (!readStoredInstall(stored_path, stored_crc, stored_size)) {
        return false;
    }
    if (stored_path != path) {
        return false;
    }

    File f = openSdAppBin(path);
    if (!f) {
        return false;
    }

    out_size = f.size();
    if (out_size == 0 || out_size != stored_size) {
        f.close();
        return false;
    }

    out_crc = crc32File(f);
    f.close();
    return out_crc == stored_crc;
}

AppInstallResult AppInstaller::installFromSd(const char *path, AppInstallProgressFn progress) {
    if (!path || !path[0]) {
        return AppInstallResult::kOpenFailed;
    }

    const esp_partition_t *part =
        esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_1, nullptr);
    if (!part) {
        return AppInstallResult::kOtaBeginFailed;
    }

    bool force_reinstall = false;
    esp_ota_img_states_t slot_state = ESP_OTA_IMG_UNDEFINED;
    if (esp_ota_get_state_partition(part, &slot_state) == ESP_OK) {
        if (slot_state == ESP_OTA_IMG_INVALID || slot_state == ESP_OTA_IMG_ABORTED) {
            force_reinstall = true;
            Serial.println("[INSTALL] ota_1 invalid/aborted — forcing reinstall");
        }
    }

    uint32_t crc = 0;
    size_t size = 0;
    if (!force_reinstall && isSameAsInstalled(path, crc, size)) {
        Serial.printf("[INSTALL] Skip flash (same image crc=0x%08X size=%u)\n", crc, (unsigned)size);
        return AppInstallResult::kSkipSameImage;
    }

    File f = openSdAppBin(path);
    if (!f) {
        Serial.printf("[INSTALL] Cannot open: %s\n", path);
        return AppInstallResult::kOpenFailed;
    }

    const size_t image_size = f.size();
    if (image_size == 0) {
        f.close();
        return AppInstallResult::kInvalidImage;
    }

    if (!validateImageHeader(f)) {
        f.close();
        Serial.println("[INSTALL] Invalid image (bad magic)");
        return AppInstallResult::kInvalidImage;
    }

    if (image_size > part->size) {
        f.close();
        return AppInstallResult::kTooLarge;
    }

    esp_ota_handle_t ota_handle = 0;
    esp_err_t err = esp_ota_begin(part, OTA_SIZE_UNKNOWN, &ota_handle);
    if (err != ESP_OK) {
        f.close();
        Serial.printf("[INSTALL] esp_ota_begin failed: %d\n", (int)err);
        return AppInstallResult::kOtaBeginFailed;
    }

    if (!f.seek(0)) {
        esp_ota_abort(ota_handle);
        f.close();
        return AppInstallResult::kOpenFailed;
    }

    uint32_t running_crc = 0;
    uint8_t buf[4096];
    size_t offset = 0;
    size_t last_ui = 0;

    while (f.available()) {
        const size_t n = f.read(buf, sizeof(buf));
        if (n == 0) {
            break;
        }
        running_crc = esp_rom_crc32_le(running_crc, buf, n);
        err = esp_ota_write(ota_handle, buf, n);
        if (err != ESP_OK) {
            esp_ota_abort(ota_handle);
            f.close();
            Serial.printf("[INSTALL] esp_ota_write failed: %d\n", (int)err);
            return AppInstallResult::kWriteFailed;
        }
        offset += n;
        if (progress && (offset - last_ui >= 32768 || offset == image_size)) {
            progress("Copying SD -> ota_1...", offset, image_size);
            last_ui = offset;
        }
    }
    f.close();

    if (offset != image_size) {
        esp_ota_abort(ota_handle);
        return AppInstallResult::kWriteFailed;
    }

    if (progress) {
        progress("Verifying image...", image_size, image_size);
    }

    err = esp_ota_end(ota_handle);
    if (err != ESP_OK) {
        Serial.printf("[INSTALL] esp_ota_end failed: %d\n", (int)err);
        return AppInstallResult::kOtaEndFailed;
    }

    storeInstall(path, running_crc, image_size);

    err = esp_ota_set_boot_partition(part);
    if (err != ESP_OK) {
        Serial.printf("[INSTALL] set_boot_partition failed: %d\n", (int)err);
        return AppInstallResult::kBootSetFailed;
    }

    Serial.printf("[INSTALL] OK crc=0x%08X size=%u (pending verify until app confirms)\n",
                  running_crc,
                  (unsigned)image_size);
    return AppInstallResult::kOk;
}

const char *AppInstaller::resultMessage(AppInstallResult r) {
    switch (r) {
        case AppInstallResult::kOk:
            return "Install complete";
        case AppInstallResult::kSkipSameImage:
            return "Same app already installed";
        case AppInstallResult::kOpenFailed:
            return "Cannot open app.bin on SD";
        case AppInstallResult::kInvalidImage:
            return "Invalid app.bin (bad header)";
        case AppInstallResult::kTooLarge:
            return "app.bin too large for ota_1";
        case AppInstallResult::kOtaBeginFailed:
            return "OTA begin failed";
        case AppInstallResult::kWriteFailed:
            return "Flash write failed";
        case AppInstallResult::kOtaEndFailed:
            return "Image verify failed";
        case AppInstallResult::kBootSetFailed:
            return "Could not set boot slot";
        default:
            return "Unknown error";
    }
}

}  // namespace rakos

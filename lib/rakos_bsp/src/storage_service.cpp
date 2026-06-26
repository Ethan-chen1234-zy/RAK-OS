#include <rakos/storage_service.h>
#include <rakos/i2c_bus.h>
#include <rakos/pin_config.h>
#include <rakos/io_expander.h>
#include <rakos/lcd5_expander.h>
#include <rakos/sd_cs_expander.h>
#include <rakos/sd_fs.h>
#include <FS.h>
#if RAKOS_SD_USE_SPI
#include <SD.h>
#include <SPI.h>
#else
#include <SD_MMC.h>
#endif
#include <esp_spiffs.h>

namespace {

#if RAKOS_SD_USE_SPI
bool mountSpiSd() {
#if defined(RAKOS_BOARD_WAVESHARE_LCD5) || defined(RAKOS_BOARD_WAVESHARE_LCD5B)
    if (!rakos::lcd5_expander_ready()) {
        Serial.println("[BSP] SPI SD: CH422G not ready (panel not up?)");
        return false;
    }

    // Waveshare 03_SD_Test: CS on CH422G EXIO4, held LOW for dedicated SPI SD bus.
    rakos_sd_use_expander_cs(true);
    if (rakos::lcd5_expander_lock(200)) {
        rakos::lcd5_expander()->digitalWrite(EXPIO_SD_CS, LOW);
        rakos::lcd5_expander_unlock();
    }

    SPI.setHwCs(false);
    SPI.begin(SD_PIN_CLK, SD_PIN_MISO, SD_PIN_MOSI, SD_PIN_CS);

    if (!SD.begin(SD_PIN_CS, SPI, 16000000, SD_MOUNT_POINT, SD_MAX_FILES)) {
        Serial.println("[BSP] SD mount failed (check TF card, FAT32)");
        return false;
    }
    if (SD.cardType() == CARD_NONE || SD.cardSize() == 0) {
        Serial.println("[BSP] No SD card detected");
        SD.end();
        return false;
    }

    Serial.printf("[BSP] SD mounted at %s, size=%lluMB, type=%u\n",
                  SD_MOUNT_POINT,
                  SD.cardSize() / (1024ULL * 1024ULL),
                  static_cast<unsigned>(SD.cardType()));
    return true;
#else
    (void)0;
    Serial.println("[BSP] SPI SD mount not configured for this board");
    return false;
#endif
}
#endif

}  // namespace

bool StorageService::probeSdPresent() const {
#if RAKOS_SD_USE_SPI
    if (!sd_ok_) {
        return false;
    }
    return SD.cardType() != CARD_NONE && SD.cardSize() > 0;
#else
    if (SD_MMC.cardType() == CARD_NONE || SD_MMC.cardSize() == 0) {
        return false;
    }

    static const char *kProbeRoots[] = {"/", "/sdcard", SD_FS_ROOT};
    for (const char *path : kProbeRoots) {
        File root = SD_MMC.open(path);
        if (root && root.isDirectory()) {
            root.close();
            return SD_MMC.totalBytes() > 0;
        }
        if (root) {
            root.close();
        }
    }

    // Card stats OK but no dir opened — still treat as present (avoid false unmount).
    return SD_MMC.totalBytes() > 0;
#endif
}

void StorageService::unmountSd() {
    if (!sd_ok_) {
        return;
    }
#if RAKOS_SD_USE_SPI
    SD.end();
#else
    SD_MMC.end();
#endif
    sd_ok_ = false;
    Serial.println("[BSP] SD unmounted (removed or unreadable)");
}

bool StorageService::updateSdState() {
    const bool was_ok = sd_ok_;

    if (sd_ok_) {
        if (!probeSdPresent()) {
            unmountSd();
        }
    } else {
        tryMountSd();
    }

    return sd_ok_ != was_ok;
}

uint64_t StorageService::sdSizeMb() const {
    if (!sd_ok_) {
        return 0;
    }
#if RAKOS_SD_USE_SPI
    return SD.cardSize() / (1024ULL * 1024ULL);
#else
    return SD_MMC.cardSize() / (1024ULL * 1024ULL);
#endif
}

bool StorageService::init() {
    esp_vfs_spiffs_conf_t spiffs_conf = {
        .base_path = SPIFFS_MOUNT_POINT,
        .partition_label = "storage",
        .max_files = 5,
        .format_if_mount_failed = true,
    };
    if (esp_vfs_spiffs_register(&spiffs_conf) == ESP_OK) {
        spiffs_ok_ = true;
        Serial.println("[BSP] SPIFFS mounted on storage partition");
    } else {
        Serial.println("[BSP] SPIFFS mount failed");
    }

#if RAKOS_SD_USE_SPI
    if (mountSpiSd()) {
        sd_ok_ = true;
    }
#else
    auto &expander = rakos::IoExpander::instance();
    if (expander.ready()) {
        expander.pinMode(EXPIO_SD_CS, OUTPUT);
        expander.digitalWrite(EXPIO_SD_CS, HIGH);
        delay(1000);
    } else {
        Serial.println("[BSP] SD enable skipped (IO expander not ready)");
    }

    if (!SD_MMC.setPins(SD_PIN_CLK, SD_PIN_CMD, SD_PIN_D0)) {
        Serial.println("[BSP] SD_MMC setPins failed");
        return spiffs_ok_;
    }

    if (SD_MMC.begin(SD_MOUNT_POINT, true, false, SDMMC_FREQ_DEFAULT, SD_MAX_FILES)) {
        sd_ok_ = true;
        Serial.printf("[BSP] SD mounted, size=%lluMB\n", SD_MMC.cardSize() / (1024ULL * 1024ULL));
    } else {
        Serial.println("[BSP] SD mount failed (check TF card)");
    }
#endif

    return sd_ok_ || spiffs_ok_;
}

bool StorageService::tryMountSd() {
    if (sd_ok_ && probeSdPresent()) {
        return true;
    }
    if (sd_ok_) {
        unmountSd();
    }

#if RAKOS_SD_USE_SPI
    if (mountSpiSd()) {
        sd_ok_ = true;
    }
    return sd_ok_;
#else
    auto &expander = rakos::IoExpander::instance();
    if (expander.ready()) {
        expander.pinMode(EXPIO_SD_CS, OUTPUT);
        expander.digitalWrite(EXPIO_SD_CS, HIGH);
        delay(200);
    }

    SD_MMC.end();

    if (!SD_MMC.setPins(SD_PIN_CLK, SD_PIN_CMD, SD_PIN_D0)) {
        Serial.println("[BSP] SD_MMC setPins failed (retry)");
        return false;
    }

    if (SD_MMC.begin(SD_MOUNT_POINT, true, false, SDMMC_FREQ_DEFAULT, SD_MAX_FILES)) {
        sd_ok_ = true;
        Serial.printf("[BSP] SD mounted on retry, size=%lluMB\n", SD_MMC.cardSize() / (1024ULL * 1024ULL));
    } else {
        Serial.println("[BSP] SD mount retry failed");
    }
    return sd_ok_;
#endif
}

void StorageService::recoverI2cAfterMount() {
#if !defined(RAKOS_BOARD_WAVESHARE_LCD5) && !defined(RAKOS_BOARD_WAVESHARE_LCD5B)
    rakos::i2cBusRecover();
#endif
}

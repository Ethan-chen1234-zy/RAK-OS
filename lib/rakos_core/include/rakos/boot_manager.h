#pragma once

#include <Arduino.h>
#include <esp_partition.h>

struct PartitionSummary {
    const esp_partition_t *running = nullptr;
    const esp_partition_t *boot = nullptr;
    const esp_partition_t *ota0 = nullptr;
    const esp_partition_t *ota1 = nullptr;
};

class BootManager {
public:
    static void begin();
    static PartitionSummary getSummary();
    static bool isRunningOsSlot();
    static bool isRunningAppSlot();
    static bool setNextBootOs();
    static bool setNextBootApp();
    /** True when ota_1 contains a valid ESP32 app image (e.g. USB flash @ 0x400000). */
    static bool ota1HasFirmware();
    static void reboot();
    static void logSummary();
    static int getOtaState(const esp_partition_t *part);

    /** True once after ESP-IDF rolled back from a bad ota_1 boot to ota_0. */
    static bool consumeAppRollbackNotice();

    /** ota_1 image is pending first-boot confirmation. */
    static bool isOta1PendingVerify();

private:
    static void detectAppRollbackOnOsBoot();
};

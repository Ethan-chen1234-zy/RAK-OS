#pragma once

#include <Arduino.h>

class StorageService {
public:
    bool init();
    /** Retry SD mount after hot-insert (no-op if already mounted). */
    bool tryMountSd();
    /** Poll SD presence; returns true if mounted state changed. */
    bool updateSdState();
    bool sdReady() const { return sd_ok_; }
    uint64_t sdSizeMb() const;
    bool spiffsReady() const { return spiffs_ok_; }
    void recoverI2cAfterMount();

private:
    bool sd_ok_ = false;
    bool spiffs_ok_ = false;

    bool probeSdPresent() const;
    void unmountSd();
};

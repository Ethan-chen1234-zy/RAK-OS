#pragma once

#include <Arduino.h>
#include <functional>

namespace rakos {

enum class AppInstallResult {
    kOk,
    kSkipSameImage,
    kOpenFailed,
    kInvalidImage,
    kTooLarge,
    kOtaBeginFailed,
    kWriteFailed,
    kOtaEndFailed,
    kBootSetFailed,
};

using AppInstallProgressFn = std::function<void(const char *phase, size_t done, size_t total)>;

class AppInstaller {
public:
    /** Install SD app.bin to ota_1 via esp_ota_* (incremental erase, validated end). */
    static AppInstallResult installFromSd(const char *path, AppInstallProgressFn progress = nullptr);

    /** True if SD file matches last successful install (skip reflash). */
    static bool isSameAsInstalled(const char *path, uint32_t &out_crc, size_t &out_size);

    static const char *resultMessage(AppInstallResult r);
};

}  // namespace rakos

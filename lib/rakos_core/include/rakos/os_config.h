#pragma once

#include <Arduino.h>

enum class OsMode : uint8_t {
    kMaker = 0,
    kManaged = 1,
};

struct OsConfigData {
    OsMode mode = OsMode::kManaged;
    uint8_t default_boot_subtype = 0;
    uint32_t boot_counter = 0;
};

class OsConfig {
public:
    bool begin();
    const OsConfigData &data() const { return cfg_; }
    void setMode(OsMode mode);
    void setDefaultBootSubtype(uint8_t subtype);
    void save();

private:
    OsConfigData cfg_;
};

#pragma once

#include <FS.h>
#include <rakos/pin_config.h>

#if RAKOS_SD_USE_SPI
#include <SD.h>
#else
#include <SD_MMC.h>
#endif

namespace rakos {

inline fs::FS &sdFs() {
#if RAKOS_SD_USE_SPI
    return SD;
#else
    return SD_MMC;
#endif
}

}  // namespace rakos

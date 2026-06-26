#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Route Arduino SD chip-select to CH422G (Waveshare LCD-5). */
void rakos_sd_use_expander_cs(bool enable);

bool rakos_sd_cs_external(void);
void rakos_sd_cs_set(int level);

#ifdef __cplusplus
}
#endif

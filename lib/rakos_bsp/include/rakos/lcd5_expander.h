#pragma once

#if defined(RAKOS_BOARD_WAVESHARE_LCD5) || defined(RAKOS_BOARD_WAVESHARE_LCD5B)

#include <esp_io_expander.hpp>

namespace rakos {

/** Bind CH422G from ESP Panel after board->begin(). */
void lcd5_bind_expander(esp_expander::Base *expander);
esp_expander::Base *lcd5_expander();
bool lcd5_expander_ready();
/** Serialize CH422G I2C access (shared bus with GT911 touch). */
bool lcd5_expander_lock(uint32_t timeout_ms = 50);
void lcd5_expander_unlock();

}  // namespace rakos

#endif

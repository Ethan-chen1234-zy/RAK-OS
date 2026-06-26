#if defined(RAKOS_BOARD_WAVESHARE_LCD5) || defined(RAKOS_BOARD_WAVESHARE_LCD5B)



#include <rakos/sd_cs_expander.h>

#include <rakos/lcd5_expander.h>

#include <rakos/pin_config.h>

#include <Arduino.h>



static bool g_use_expander_cs = false;



extern "C" void rakos_sd_use_expander_cs(bool enable) {

    g_use_expander_cs = enable;

}



extern "C" bool rakos_sd_cs_external(void) {

    return g_use_expander_cs && rakos::lcd5_expander_ready();

}



extern "C" void rakos_sd_cs_set(int level) {

    (void)level;

    // Waveshare 03_SD_Test: assert SD_CS LOW once before SD.begin(); do not

    // toggle via I2C per SPI transaction (floods CH422G / contends with touch).

}



#endif


#if defined(RAKOS_BOARD_WAVESHARE_LCD5) || defined(RAKOS_BOARD_WAVESHARE_LCD5B)



#include <rakos/lcd5_expander.h>

#include <freertos/FreeRTOS.h>

#include <freertos/semphr.h>



namespace rakos {



static esp_expander::Base *g_lcd5_expander = nullptr;

static SemaphoreHandle_t g_expander_mutex = nullptr;



static void ensure_expander_mutex() {

    if (!g_expander_mutex) {

        g_expander_mutex = xSemaphoreCreateMutex();

    }

}



void lcd5_bind_expander(esp_expander::Base *expander) {

    g_lcd5_expander = expander;

}



esp_expander::Base *lcd5_expander() {

    return g_lcd5_expander;

}



bool lcd5_expander_ready() {

    return g_lcd5_expander != nullptr;

}



bool lcd5_expander_lock(uint32_t timeout_ms) {

    if (!g_lcd5_expander) {

        return false;

    }

    ensure_expander_mutex();

    return xSemaphoreTake(g_expander_mutex, pdMS_TO_TICKS(timeout_ms)) == pdTRUE;

}



void lcd5_expander_unlock() {

    if (g_expander_mutex) {

        xSemaphoreGive(g_expander_mutex);

    }

}



}  // namespace rakos



#endif


#if defined(RAKOS_BOARD_WAVESHARE_LCD5) || defined(RAKOS_BOARD_WAVESHARE_LCD5B)

#include <rakos/display_manager.h>
#include <rakos/io_expander.h>
#include <rakos/lcd5_expander.h>
#include <rakos/pin_config.h>
#include <Preferences.h>

#include <esp_display_panel.hpp>
#include "esp_panel_board_custom_conf.h"
#include "lvgl_v8_port.h"

using namespace esp_panel::board;
using namespace esp_panel::drivers;

static Preferences prefs;
static Board *g_board = nullptr;

DisplayManager::DisplayManager() = default;

DisplayManager::~DisplayManager() {
    if (g_board) {
        delete g_board;
        g_board = nullptr;
    }
}

bool DisplayManager::initTouch() {
    return g_board && g_board->getTouch();
}

bool DisplayManager::init() {
    Serial.println("[BSP] Display bring-up (Waveshare LCD-5 RGB)...");

    prefs.begin("rakos_ui", false);

    g_board = new Board();
    g_board->init();

#if LVGL_PORT_AVOID_TEARING_MODE
    auto lcd = g_board->getLCD();
    lcd->configFrameBufferNumber(LVGL_PORT_DISP_BUFFER_NUM);
#if ESP_PANEL_DRIVERS_BUS_ENABLE_RGB && CONFIG_IDF_TARGET_ESP32S3
    auto lcd_bus = lcd->getBus();
    if (lcd_bus->getBasicAttributes().type == ESP_PANEL_BUS_TYPE_RGB) {
        static_cast<BusRGB *>(lcd_bus)->configRGB_BounceBufferSize(ESP_PANEL_BOARD_LCD_RGB_BOUNCE_BUF_SIZE);
    }
#endif
#endif

    if (!g_board->begin()) {
        Serial.println("[BSP] ESP Panel board init failed");
        delete g_board;
        g_board = nullptr;
        return false;
    }

    if (g_board->getIO_Expander()) {
        rakos::lcd5_bind_expander(g_board->getIO_Expander()->getBase());
        rakos::IoExpander::instance().begin();
    }

    if (!lvgl_port_init(g_board->getLCD(), g_board->getTouch())) {
        Serial.println("[BSP] LVGL port init failed");
        return false;
    }

    brightness_pct_ = getBrightnessPercentage();
    setBrightness((uint8_t)(((uint16_t)brightness_pct_ * 255 + 50) / 100));
    panel_board_ = g_board;
    Serial.println("[BSP] LCD-5 display ready");
    return true;
}

void DisplayManager::startLvglTick() {}

void DisplayManager::runLvgl() {
    if (!lvgl_port_is_ready() || !lvgl_port_lock(-1)) {
        return;
    }
    lv_timer_handler();
    lvgl_port_unlock();
}

void DisplayManager::update() {
    runLvgl();
}

void DisplayManager::forceRefresh() {
    // LVGL timer task owns refresh; lv_refr_now() here caused RGB tearing (top-left stripes).
}

void DisplayManager::setBrightness(uint8_t brightness, bool persist) {
    uint8_t pct = (uint8_t)(((uint16_t)brightness * 100 + 127) / 255);
    if (pct > 100) {
        pct = 100;
    }
    brightness_pct_ = pct;

    auto *exp = rakos::lcd5_expander();
    if (exp && rakos::lcd5_expander_lock(100)) {
        exp->digitalWrite(EXPIO_LCD_BL, pct > 0 ? HIGH : LOW);
        rakos::lcd5_expander_unlock();
    }

    if (persist) {
        prefs.putUChar("brightness_pct", pct);
    }
}

uint8_t DisplayManager::getBrightnessPercentage() {
    uint8_t pct = prefs.getUChar("brightness_pct", 80);
    if (pct == 0) {
        pct = 80;
        prefs.putUChar("brightness_pct", 80);
    }
    return pct > 100 ? 100 : pct;
}

bool DisplayManager::getTouchCoordinates(int16_t &x, int16_t &y) {
    (void)x;
    (void)y;
    return false;
}

#endif

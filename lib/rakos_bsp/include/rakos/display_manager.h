#pragma once

#include <Arduino.h>
#include <lvgl.h>
#include <memory>
#include <rakos/pin_config.h>

#if !defined(RAKOS_BOARD_WAVESHARE_LCD5) && !defined(RAKOS_BOARD_WAVESHARE_LCD5B)
#include <Arduino_GFX_Library.h>
#include <Arduino_DriveBus_Library.h>
#if defined(RAKOS_BOARD_PY206_W38_V2)
#include <touch/TouchDrvCST92xx.hpp>
#endif
#if defined(RAKOS_PANEL_CO5300)
using RakosPanel = Arduino_CO5300;
#else
using RakosPanel = Arduino_SH8601;
#endif
#endif

class DisplayManager {
public:
    DisplayManager();
    ~DisplayManager();

    bool init();
    bool initTouch();
    void startLvglTick();
    void runLvgl();
    void update();
    void forceRefresh();

#if !defined(RAKOS_BOARD_WAVESHARE_LCD5) && !defined(RAKOS_BOARD_WAVESHARE_LCD5B)
    RakosPanel *getGfx() { return gfx; }
#endif

    void setBrightness(uint8_t brightness, bool persist = true);
    void setBrightnessPercent(uint8_t pct, bool persist = true);
    uint8_t getBrightnessPercentage();
    bool getTouchCoordinates(int16_t &x, int16_t &y);

private:
#if defined(RAKOS_BOARD_WAVESHARE_LCD5) || defined(RAKOS_BOARD_WAVESHARE_LCD5B)
    void *panel_board_{nullptr};
    uint8_t brightness_pct_{80};
#else
    Arduino_DataBus *bus;
    RakosPanel *gfx;
#if defined(RAKOS_BOARD_PY206_W38_V2)
    std::unique_ptr<TouchDrvCST92xx> touch_cst92xx_;
#else
    std::shared_ptr<Arduino_IIC_DriveBus> touch_bus_;
    std::unique_ptr<Arduino_IIC> touch_;
#endif
    lv_display_t *display;
    lv_color_t *buf;
    lv_color_t *buf2;

    bool readTouch(int16_t &x, int16_t &y);

    static DisplayManager *instance;
    static void disp_flush_callback(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map);
    static void touchpad_read_callback(lv_indev_t *indev, lv_indev_data_t *data);
    static void display_rounder_event_cb(lv_event_t *e);
#endif
};

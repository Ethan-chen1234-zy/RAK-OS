#pragma once

#include <Arduino.h>
#include <lvgl.h>
#include <memory>
#include <rakos/pin_config.h>

#if !defined(RAKOS_BOARD_WAVESHARE_LCD5) && !defined(RAKOS_BOARD_WAVESHARE_LCD5B)
#include <Arduino_GFX_Library.h>
#include <Arduino_DriveBus_Library.h>
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
    Arduino_SH8601 *getGfx() { return gfx; }
#endif

    void setBrightness(uint8_t brightness, bool persist = true);
    uint8_t getBrightnessPercentage();
    bool getTouchCoordinates(int16_t &x, int16_t &y);

private:
#if defined(RAKOS_BOARD_WAVESHARE_LCD5) || defined(RAKOS_BOARD_WAVESHARE_LCD5B)
    void *panel_board_{nullptr};
    uint8_t brightness_pct_{80};
#else
    Arduino_DataBus *bus;
    Arduino_SH8601 *gfx;
    std::shared_ptr<Arduino_IIC_DriveBus> touch_bus_;
    std::unique_ptr<Arduino_IIC> touch_;
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

/**
 * LVGL 8.4 config for Waveshare ESP32-S3-Touch-LCD-5 (RAKOS rakos_os_lcd5)
 */
#ifndef RAKOS_LVGL8_CONF_H
#define RAKOS_LVGL8_CONF_H

#include <stdint.h>

#define LV_COLOR_DEPTH 16
#define LV_COLOR_16_SWAP 0

#define LV_MEM_CUSTOM 0
#define LV_MEM_SIZE (128 * 1024U)

#define LV_USE_LOG 0
#define LV_USE_PERF_MONITOR 0
#define LV_USE_MEM_MONITOR 0

#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_16 1
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_24 1
#define LV_FONT_MONTSERRAT_30 1
#define LV_FONT_DEFAULT &lv_font_montserrat_20

#define LV_USE_BTN 1
#define LV_USE_LABEL 1
#define LV_USE_LINE 1
#define LV_USE_LIST 1
#define LV_USE_FLEX 1
#define LV_USE_SLIDER 1
#define LV_USE_BAR 1
#define LV_USE_MSGBOX 1
#define LV_USE_SWITCH 1
#define LV_USE_TEXTAREA 1
#define LV_USE_KEYBOARD 1
#define LV_USE_THEME_DEFAULT 1
#if LV_USE_THEME_DEFAULT
#define LV_THEME_DEFAULT_DARK 1
#endif

#endif

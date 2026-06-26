/**
 * @file lv_conf.h
 * LVGL config router (v9 AMOLED / v8 LCD-5)
 */
#ifndef LV_CONF_H
#define LV_CONF_H

#if defined(RAKOS_LVGL8)
#include "rakos_lvgl8_conf.h"
#else

#include <stdint.h>

#define LV_COLOR_DEPTH 16
#define LV_COLOR_16_SWAP 0

#define LV_USE_STDLIB_MALLOC    LV_STDLIB_BUILTIN
#define LV_USE_STDLIB_STRING    LV_STDLIB_BUILTIN
#define LV_USE_STDLIB_SPRINTF   LV_STDLIB_BUILTIN
#define LV_MEM_SIZE (64 * 1024U)

#define LV_USE_DRAW_SW_ASM LV_DRAW_SW_ASM_NONE
#define LV_USE_NATIVE_HELIUM_ASM 0

#define LV_USE_LOG 0
#define LV_USE_PERF_MONITOR 0
#define LV_USE_MEM_MONITOR 0

#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_16 1
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_24 1
#define LV_FONT_DEFAULT &lv_font_montserrat_20

#define LV_USE_BUTTON 1
#define LV_USE_LABEL 1
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

#endif /* !RAKOS_LVGL8 */
#endif /* LV_CONF_H */

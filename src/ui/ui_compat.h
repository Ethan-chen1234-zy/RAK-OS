#pragma once

#include "ui_msgbox.h"

#if defined(RAKOS_LVGL8)

#define lv_screen_active() lv_scr_act()
#define lv_button_create   lv_btn_create
#define lv_obj_delete      lv_obj_del
#define lv_obj_remove_flag lv_obj_clear_flag
#define lv_list_add_button lv_list_add_btn

#define UI_FONT_BODY   lv_font_montserrat_20
#define UI_FONT_TITLE  lv_font_montserrat_30
#define UI_FONT_ICON   lv_font_montserrat_30

#else

#define UI_FONT_BODY   Inter_20
#define UI_FONT_TITLE  Inter_30
#define UI_FONT_ICON   Inter_30

#endif

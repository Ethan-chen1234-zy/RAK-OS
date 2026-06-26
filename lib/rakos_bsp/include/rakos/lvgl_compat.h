#pragma once

#include <lvgl.h>

#if defined(RAKOS_LVGL8)

#define lv_screen_active() lv_scr_act()
#define lv_button_create   lv_btn_create
#define lv_obj_delete      lv_obj_del
#define lv_list_add_button lv_list_add_btn

typedef lv_point_t lv_point_precise_t;

#endif

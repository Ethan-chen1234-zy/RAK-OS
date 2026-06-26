#pragma once

#include <lvgl.h>

#if defined(RAKOS_LVGL8)

#define lv_screen_active() lv_scr_act()
#define lv_button_create lv_btn_create
#define lv_obj_delete lv_obj_del
#define lv_obj_remove_flag lv_obj_clear_flag

#endif

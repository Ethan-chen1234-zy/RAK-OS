#pragma once

#include <lvgl.h>

lv_obj_t *ui_msgbox_create_ok(lv_obj_t *parent, const char *title, const char *text);
lv_obj_t *ui_msgbox_create_yes_no(lv_obj_t *parent, const char *title, const char *text,
                                  const char *yes_label, const char *no_label);
lv_obj_t *ui_msgbox_get_from_button(lv_obj_t *btn);
void ui_msgbox_close_box(lv_obj_t *mbox);

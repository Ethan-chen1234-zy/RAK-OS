#pragma once

#include <lvgl.h>

extern const lv_font_t Inter_20;
extern const lv_font_t Inter_30;

void ui_theme_init();

lv_color_t ui_color_bg();
lv_color_t ui_color_card();
lv_color_t ui_color_text();
lv_color_t ui_color_muted();
lv_color_t ui_color_accent();
lv_color_t ui_color_accent_dim();
lv_color_t ui_color_ok();
lv_color_t ui_color_danger();

void ui_style_screen(lv_obj_t *obj);
/** Scrollable page column inside content_area_ */
void ui_style_page(lv_obj_t *obj);
void ui_style_card(lv_obj_t *obj);
void ui_style_card_subtle(lv_obj_t *obj);

lv_obj_t *ui_add_separator(lv_obj_t *parent);
lv_obj_t *ui_make_primary_btn(lv_obj_t *parent, const char *text, lv_event_cb_t cb);
lv_obj_t *ui_make_outline_btn(lv_obj_t *parent, const char *text, lv_event_cb_t cb);
void ui_style_title_label(lv_obj_t *label);
void ui_style_body_label(lv_obj_t *label);
void ui_style_muted_label(lv_obj_t *label);
void ui_style_accent_label(lv_obj_t *label);

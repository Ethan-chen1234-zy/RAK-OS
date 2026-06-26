#include "ui_theme.h"
#include "ui_compat.h"

static bool g_theme_ready = false;

lv_color_t ui_color_bg() { return lv_color_hex(0x000000); }
lv_color_t ui_color_card() { return lv_color_hex(0x111111); }
lv_color_t ui_color_text() { return lv_color_hex(0xFFFAF5); }
lv_color_t ui_color_muted() { return lv_color_hex(0x9A948F); }
lv_color_t ui_color_accent() { return lv_color_hex(0xFF7F1F); }
lv_color_t ui_color_accent_dim() { return lv_color_hex(0xCC6619); }
lv_color_t ui_color_ok() { return lv_color_hex(0x00FF88); }
lv_color_t ui_color_danger() { return lv_color_hex(0xE53E3E); }

void ui_theme_init() {
    if (g_theme_ready) {
        return;
    }
    g_theme_ready = true;
}

void ui_style_screen(lv_obj_t *obj) {
    lv_obj_remove_style_all(obj);
    lv_obj_set_size(obj, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(obj, ui_color_bg(), 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
}

void ui_style_page(lv_obj_t *obj) {
    lv_obj_remove_style_all(obj);
    lv_obj_set_width(obj, lv_pct(100));
    lv_obj_set_height(obj, LV_SIZE_CONTENT);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(obj, LV_FLEX_FLOW_COLUMN);
}

void ui_style_card(lv_obj_t *obj) {
    lv_obj_set_style_bg_color(obj, ui_color_card(), 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_60, 0);
    lv_obj_set_style_radius(obj, 16, 0);
    lv_obj_set_style_border_width(obj, 1, 0);
    lv_obj_set_style_border_color(obj, lv_color_hex(0x2A2A2A), 0);
    lv_obj_set_style_border_opa(obj, LV_OPA_50, 0);
    lv_obj_set_style_pad_all(obj, 14, 0);
}

void ui_style_card_subtle(lv_obj_t *obj) {
    ui_style_card(obj);
    lv_obj_set_style_bg_opa(obj, LV_OPA_20, 0);
    lv_obj_set_style_border_color(obj, ui_color_muted(), 0);
    lv_obj_set_style_border_opa(obj, LV_OPA_30, 0);
}

lv_obj_t *ui_add_separator(lv_obj_t *parent) {
    lv_obj_t *line = lv_obj_create(parent);
    lv_obj_remove_style_all(line);
    lv_obj_set_width(line, lv_pct(100));
    lv_obj_set_height(line, 1);
    lv_obj_set_style_bg_color(line, ui_color_muted(), 0);
    lv_obj_set_style_bg_opa(line, LV_OPA_30, 0);
    return line;
}

static void style_primary_btn(lv_obj_t *btn) {
    lv_obj_set_style_bg_color(btn, ui_color_accent(), 0);
    lv_obj_set_style_bg_color(btn, ui_color_accent_dim(), LV_STATE_PRESSED);
    lv_obj_set_style_radius(btn, 24, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
}

lv_obj_t *ui_make_primary_btn(lv_obj_t *parent, const char *text, lv_event_cb_t cb) {
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_width(btn, lv_pct(100));
    lv_obj_set_height(btn, 48);
    style_primary_btn(btn);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, nullptr);

    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_font(lbl, &UI_FONT_BODY, 0);
    lv_obj_set_style_text_color(lbl, ui_color_bg(), 0);
    lv_obj_center(lbl);
    lv_obj_remove_flag(lbl, LV_OBJ_FLAG_CLICKABLE);
    return btn;
}

lv_obj_t *ui_make_outline_btn(lv_obj_t *parent, const char *text, lv_event_cb_t cb) {
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_width(btn, lv_pct(100));
    lv_obj_set_height(btn, 44);
    lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(btn, 1, 0);
    lv_obj_set_style_border_color(btn, ui_color_muted(), 0);
    lv_obj_set_style_radius(btn, 22, 0);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, nullptr);

    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_font(lbl, &UI_FONT_BODY, 0);
    lv_obj_set_style_text_color(lbl, ui_color_text(), 0);
    lv_obj_center(lbl);
    lv_obj_remove_flag(lbl, LV_OBJ_FLAG_CLICKABLE);
    return btn;
}

void ui_style_title_label(lv_obj_t *label) {
    lv_obj_set_style_text_color(label, ui_color_text(), 0);
    lv_obj_set_style_text_font(label, &UI_FONT_TITLE, 0);
}

void ui_style_body_label(lv_obj_t *label) {
    lv_obj_set_style_text_color(label, ui_color_text(), 0);
    lv_obj_set_style_text_font(label, &UI_FONT_BODY, 0);
}

void ui_style_muted_label(lv_obj_t *label) {
    lv_obj_set_style_text_color(label, ui_color_muted(), 0);
    lv_obj_set_style_text_font(label, &UI_FONT_BODY, 0);
}

void ui_style_accent_label(lv_obj_t *label) {
    lv_obj_set_style_text_color(label, ui_color_accent(), 0);
    lv_obj_set_style_text_font(label, &UI_FONT_BODY, 0);
}

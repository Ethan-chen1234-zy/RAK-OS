#include "ui_msgbox.h"

#if defined(RAKOS_LVGL8)

lv_obj_t *ui_msgbox_create_ok(lv_obj_t *parent, const char *title, const char *text) {
    static const char *btns[] = {"OK", ""};
    lv_obj_t *mbox = lv_msgbox_create(parent, title, text, btns, true);
    return mbox;
}

lv_obj_t *ui_msgbox_create_yes_no(lv_obj_t *parent, const char *title, const char *text,
                                  const char *yes_label, const char *no_label) {
    static const char *btns[3];
    btns[0] = yes_label;
    btns[1] = no_label;
    btns[2] = "";
    return lv_msgbox_create(parent, title, text, btns, true);
}

lv_obj_t *ui_msgbox_get_from_button(lv_obj_t *btn) {
    return lv_obj_get_parent(btn);
}

void ui_msgbox_close_box(lv_obj_t *mbox) {
    lv_msgbox_close(mbox);
}

#else

lv_obj_t *ui_msgbox_create_ok(lv_obj_t *parent, const char *title, const char *text) {
    lv_obj_t *mbox = lv_msgbox_create(parent);
    lv_msgbox_add_title(mbox, title);
    lv_msgbox_add_text(mbox, text);
    lv_msgbox_add_footer_button(mbox, "OK");
    return mbox;
}

lv_obj_t *ui_msgbox_create_yes_no(lv_obj_t *parent, const char *title, const char *text,
                                  const char *yes_label, const char *no_label) {
    lv_obj_t *mbox = lv_msgbox_create(parent);
    lv_msgbox_add_title(mbox, title);
    lv_msgbox_add_text(mbox, text);
    lv_msgbox_add_footer_button(mbox, yes_label);
    lv_msgbox_add_footer_button(mbox, no_label);
    return mbox;
}

lv_obj_t *ui_msgbox_get_from_button(lv_obj_t *btn) {
    return lv_obj_get_parent(lv_obj_get_parent(btn));
}

void ui_msgbox_close_box(lv_obj_t *mbox) {
    lv_msgbox_close(mbox);
}

#endif

#ifdef RAKOS_UI_APP_GRID

#include "launcher_ui.h"
#include "ui_theme.h"
#include "ui_compat.h"
#include "ui_fonts.h"
#include <rakos/app_registry.h>
#include <rakos/boot_manager.h>
#include <lvgl.h>
#include <vector>
#include <cstring>

extern LauncherUI *g_ui;

namespace {

char tile_letter(const char *name, char fallback) {
    if (!name || !name[0]) {
        return fallback;
    }
    char c = name[0];
    if (c >= 'a' && c <= 'z') {
        c = static_cast<char>(c - 'a' + 'A');
    }
    return c;
}

void abbrev_app_name(const char *src, char *dst, size_t dst_sz, int max_chars) {
    if (!dst || dst_sz == 0) {
        return;
    }
    dst[0] = '\0';
    if (!src || !src[0]) {
        return;
    }
    const size_t len = strlen(src);
    if (static_cast<int>(len) <= max_chars) {
        strncpy(dst, src, dst_sz - 1);
        dst[dst_sz - 1] = '\0';
        return;
    }
    const int keep = max_chars > 1 ? max_chars - 1 : max_chars;
    strncpy(dst, src, static_cast<size_t>(keep));
    dst[keep] = '\0';
    if (keep + 1 < static_cast<int>(dst_sz)) {
        dst[keep] = '.';
        dst[keep + 1] = '\0';
    }
}

struct GridLayout {
    int tile_w;
    int tile_h;
    int badge_sz;
    int label_w;
    int pad_col;
    int pad_row;
    int cols;
    int rows;
    int tiles_per_page;
    int viewport_h;
    int abbrev_chars;
};

GridLayout grid_layout() {
#if defined(RAKOS_BOARD_WAVESHARE_LCD5) || defined(RAKOS_BOARD_WAVESHARE_LCD5B)
    constexpr int kTileW = 120;
    constexpr int kTileH = 124;
    constexpr int kBadgeSz = 56;
    constexpr int kLabelW = 108;
    constexpr int kPadCol = 14;
    constexpr int kPadRow = 14;
    constexpr int kCols = 4;
    constexpr int kRows = 2;
#else
    constexpr int kTileW = 100;
    constexpr int kTileH = 104;
    constexpr int kBadgeSz = 48;
    constexpr int kLabelW = 88;
    constexpr int kPadCol = 10;
    constexpr int kPadRow = 10;
    constexpr int kCols = 3;
    constexpr int kRows = 2;
#endif
    const int per_page = kCols * kRows;
    const int viewport_h = kRows * kTileH + (kRows - 1) * kPadRow + 8;
    return {kTileW, kTileH, kBadgeSz, kLabelW, kPadCol, kPadRow, kCols, kRows, per_page, viewport_h, 7};
}

lv_obj_t *make_app_tile(lv_obj_t *parent, const GridLayout &g, char letter, const char *name, lv_event_cb_t cb,
                        void *user_data) {
    lv_obj_t *tile = lv_button_create(parent);
    lv_obj_set_size(tile, g.tile_w, g.tile_h);
    lv_obj_set_style_bg_color(tile, ui_color_card(), 0);
    lv_obj_set_style_bg_opa(tile, LV_OPA_70, 0);
    lv_obj_set_style_radius(tile, 18, 0);
    lv_obj_set_style_border_width(tile, 0, 0);
    lv_obj_set_style_shadow_width(tile, 0, 0);
    lv_obj_set_style_pad_all(tile, 6, 0);
#if !defined(RAKOS_LVGL8)
    lv_obj_set_style_anim_duration(tile, 0, 0);
#endif
    lv_obj_set_flex_flow(tile, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(tile, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(tile, 4, 0);
    if (cb) {
        lv_obj_add_event_cb(tile, cb, LV_EVENT_CLICKED, user_data);
    }

    lv_obj_t *badge = lv_obj_create(tile);
    lv_obj_remove_style_all(badge);
    lv_obj_set_size(badge, g.badge_sz, g.badge_sz);
    lv_obj_set_style_radius(badge, 14, 0);
    lv_obj_set_style_bg_color(badge, ui_color_accent(), 0);
    lv_obj_set_style_bg_opa(badge, LV_OPA_COVER, 0);
    lv_obj_clear_flag(badge, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(badge, LV_OBJ_FLAG_CLICKABLE);

    char letter_buf[2] = {letter, '\0'};
    lv_obj_t *ico = lv_label_create(badge);
    lv_label_set_text(ico, letter_buf);
    lv_obj_set_style_text_font(ico, &UI_FONT_ICON, 0);
    lv_obj_set_style_text_color(ico, ui_color_bg(), 0);
    lv_obj_center(ico);
    lv_obj_remove_flag(ico, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *lbl = lv_label_create(tile);
    lv_label_set_text(lbl, name);
    ui_style_body_label(lbl);
    lv_obj_set_width(lbl, g.label_w);
    lv_label_set_long_mode(lbl, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_remove_flag(lbl, LV_OBJ_FLAG_CLICKABLE);
    return tile;
}

lv_obj_t *make_system_tile(lv_obj_t *parent, char letter, const char *name, lv_color_t accent, lv_event_cb_t cb,
                           void *user_data) {
    lv_obj_t *tile = lv_button_create(parent);
    lv_obj_set_width(tile, lv_pct(48));
    lv_obj_set_height(tile, 56);
    lv_obj_set_style_bg_color(tile, ui_color_card(), 0);
    lv_obj_set_style_bg_opa(tile, LV_OPA_50, 0);
    lv_obj_set_style_radius(tile, 14, 0);
    lv_obj_set_style_border_width(tile, 2, 0);
    lv_obj_set_style_border_color(tile, accent, 0);
    lv_obj_set_style_border_opa(tile, LV_OPA_COVER, 0);
    lv_obj_set_style_shadow_width(tile, 0, 0);
    lv_obj_set_style_pad_hor(tile, 10, 0);
    lv_obj_set_flex_flow(tile, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(tile, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(tile, 8, 0);
    if (cb) {
        lv_obj_add_event_cb(tile, cb, LV_EVENT_CLICKED, user_data);
    }

    lv_obj_t *badge = lv_obj_create(tile);
    lv_obj_remove_style_all(badge);
    lv_obj_set_size(badge, 36, 36);
    lv_obj_set_style_radius(badge, 10, 0);
    lv_obj_set_style_bg_color(badge, accent, 0);
    lv_obj_set_style_bg_opa(badge, LV_OPA_80, 0);
    lv_obj_clear_flag(badge, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(badge, LV_OBJ_FLAG_CLICKABLE);

    char letter_buf[2] = {letter, '\0'};
    lv_obj_t *ico = lv_label_create(badge);
    lv_label_set_text(ico, letter_buf);
    lv_obj_set_style_text_font(ico, &UI_FONT_BODY, 0);
    lv_obj_set_style_text_color(ico, ui_color_bg(), 0);
    lv_obj_center(ico);
    lv_obj_remove_flag(ico, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *lbl = lv_label_create(tile);
    lv_label_set_text(lbl, name);
    ui_style_accent_label(lbl);
    lv_label_set_long_mode(lbl, LV_LABEL_LONG_CLIP);
    lv_obj_remove_flag(lbl, LV_OBJ_FLAG_CLICKABLE);
    return tile;
}

struct AppTileItem {
    char letter;
    char label[12];
    lv_event_cb_t cb;
    void *user_data;
};

void on_app_grid_page_scroll(lv_event_t *e) {
    lv_obj_t *viewport = static_cast<lv_obj_t *>(lv_event_get_target(e));
    lv_obj_t *indicator = static_cast<lv_obj_t *>(lv_event_get_user_data(e));
    if (!viewport || !indicator) {
        return;
    }
    const int page_h = lv_obj_get_height(viewport);
    if (page_h <= 0) {
        return;
    }
    const int page = (lv_obj_get_scroll_y(viewport) + page_h / 2) / page_h;
    const int pages = lv_obj_get_child_count(viewport);
    if (pages > 1) {
        lv_label_set_text_fmt(indicator, "%d / %d", page + 1, pages);
    }
}

} // namespace

void LauncherUI::buildAppGridPage() {
    const GridLayout g = grid_layout();

    lv_obj_t *wrap = lv_obj_create(content_area_);
    ui_style_page(wrap);
    lv_obj_set_width(wrap, lv_pct(100));
    lv_obj_set_height(wrap, LV_SIZE_CONTENT);
    lv_obj_clear_flag(wrap, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(wrap, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(wrap, 10, 0);

    lv_obj_t *title = lv_label_create(wrap);
    lv_label_set_text(title, "Applications");
    ui_style_title_label(title);

    lv_obj_t *sys_row = lv_obj_create(wrap);
    lv_obj_remove_style_all(sys_row);
    lv_obj_set_width(sys_row, lv_pct(100));
    lv_obj_set_height(sys_row, LV_SIZE_CONTENT);
    lv_obj_clear_flag(sys_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(sys_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(sys_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(sys_row, 8, 0);

    if (BootManager::ota1HasFirmware()) {
        make_system_tile(sys_row, 'F', "Flash", ui_color_ok(), on_home_run_app, nullptr);
    }
    make_system_tile(sys_row, 'S', "SD Scan", ui_color_accent(), on_app_grid_refresh, nullptr);

    std::vector<AppTileItem> items;
    if (storage_ && storage_->sdReady() && registry_) {
        items.reserve(registry_->apps().size());
        for (size_t i = 0; i < registry_->apps().size(); ++i) {
            const AppEntry &app = registry_->apps()[i];
            if (!app.has_bin) {
                continue;
            }
            AppTileItem item{};
            item.letter = tile_letter(app.name.c_str(), 'A');
            abbrev_app_name(app.name.c_str(), item.label, sizeof(item.label), g.abbrev_chars);
            item.cb = on_app_selected;
            item.user_data = reinterpret_cast<void *>(i);
            items.push_back(item);
        }
    }

    const int page_count = items.empty() ? 0 : static_cast<int>((items.size() + g.tiles_per_page - 1) / g.tiles_per_page);

    if (page_count > 0) {
        lv_obj_t *viewport = lv_obj_create(wrap);
        lv_obj_remove_style_all(viewport);
        lv_obj_set_width(viewport, lv_pct(100));
        lv_obj_set_height(viewport, g.viewport_h);
        lv_obj_set_scroll_dir(viewport, LV_DIR_VER);
        lv_obj_set_scroll_snap_y(viewport, LV_SCROLL_SNAP_START);
        lv_obj_add_flag(viewport, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_scrollbar_mode(viewport, LV_SCROLLBAR_MODE_OFF);
        lv_obj_set_style_pad_all(viewport, 0, 0);
        lv_obj_set_style_pad_row(viewport, 0, 0);

        lv_obj_t *page_indicator = lv_label_create(wrap);
        if (page_count > 1) {
            lv_label_set_text_fmt(page_indicator, "1 / %d", page_count);
            ui_style_muted_label(page_indicator);
            lv_obj_set_style_text_align(page_indicator, LV_TEXT_ALIGN_CENTER, 0);
            lv_obj_set_width(page_indicator, lv_pct(100));
            lv_obj_add_event_cb(viewport, on_app_grid_page_scroll, LV_EVENT_SCROLL_END, page_indicator);
        } else {
            lv_obj_add_flag(page_indicator, LV_OBJ_FLAG_HIDDEN);
        }

        for (int p = 0; p < page_count; ++p) {
            lv_obj_t *page = lv_obj_create(viewport);
            lv_obj_remove_style_all(page);
            lv_obj_set_width(page, lv_pct(100));
            lv_obj_set_height(page, g.viewport_h);
            lv_obj_clear_flag(page, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_flex_flow(page, LV_FLEX_FLOW_ROW_WRAP);
            lv_obj_set_style_pad_column(page, g.pad_col, 0);
            lv_obj_set_style_pad_row(page, g.pad_row, 0);
            lv_obj_set_style_pad_all(page, 0, 0);

            const size_t start = static_cast<size_t>(p) * static_cast<size_t>(g.tiles_per_page);
            const size_t end = start + static_cast<size_t>(g.tiles_per_page);
            for (size_t i = start; i < end && i < items.size(); ++i) {
                const AppTileItem &item = items[i];
                make_app_tile(page, g, item.letter, item.label, item.cb, item.user_data);
            }
        }
    } else if (page_count == 0 && (!storage_ || !storage_->sdReady())) {
        lv_obj_t *hint = lv_label_create(wrap);
        lv_label_set_text(hint, "SD not mounted - tap SD Scan after insert");
        ui_style_muted_label(hint);
        lv_label_set_long_mode(hint, LV_LABEL_LONG_WRAP);
        lv_obj_set_width(hint, lv_pct(100));
    } else if (registry_ && registry_->apps().empty()) {
        lv_obj_t *hint = lv_label_create(wrap);
        lv_label_set_text(hint, "No apps on SD. Copy to card root:\nGames/Clock/app.bin");
        ui_style_muted_label(hint);
        lv_label_set_long_mode(hint, LV_LABEL_LONG_WRAP);
        lv_obj_set_width(hint, lv_pct(100));
    }
}

void LauncherUI::on_app_grid_refresh(lv_event_t *e) {
    (void)e;
    if (!g_ui) {
        return;
    }
    g_ui->playFeedback(920);
    g_ui->refreshApps();

    if (g_ui->storage_ && !g_ui->storage_->sdReady()) {
        g_ui->showMessage("SD not mounted", "Insert TF card and tap SD Scan again.");
    } else if (g_ui->registry_ && g_ui->registry_->apps().empty()) {
        g_ui->showMessage("No apps found", "On PC, copy to SD root:\nGames/Clock/app.bin");
    }

    g_ui->showScreen(Screen::kApps);
}

#endif // RAKOS_UI_APP_GRID

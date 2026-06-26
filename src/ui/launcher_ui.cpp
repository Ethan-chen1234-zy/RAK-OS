#include "launcher_ui.h"
#include "ui_theme.h"
#include "ui_compat.h"
#include "ui_msgbox.h"
#include "ui_fonts.h"
#include <rakos/pin_config.h>
#include <rakos/wifi_sd_config.h>
#include <rakos/sd_fs.h>
#include <lvgl.h>
#include <esp_partition.h>
#include <FS.h>
#include <cstring>
#include <time.h>

LauncherUI *g_ui = nullptr;
static ImuSample g_last_imu;

static lv_obj_t *make_setting_row(lv_obj_t *parent, const char *title, const char *subtitle = nullptr) {
    lv_obj_t *row = lv_obj_create(parent);
    ui_style_card(row);
    lv_obj_set_width(row, lv_pct(100));
    lv_obj_set_height(row, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(row, 6, 0);

    lv_obj_t *t = lv_label_create(row);
    lv_label_set_text(t, title);
    ui_style_body_label(t);
    if (subtitle) {
        lv_obj_t *s = lv_label_create(row);
        lv_label_set_text(s, subtitle);
        ui_style_muted_label(s);
    }
    return row;
}

void LauncherUI::begin(DisplayManager *display,
                       InputManager *input,
                       StorageService *storage,
                       AppRegistry *registry,
                       OsConfig *config,
                       PowerService *power,
                       ImuService *imu,
                       AudioService *audio,
                       RadioService *radio) {
    g_ui = this;
    display_ = display;
    input_ = input;
    storage_ = storage;
    registry_ = registry;
    config_ = config;
    power_ = power;
    imu_ = imu;
    audio_ = audio;
    radio_ = radio;

    ui_theme_init();

    if (display_) {
        saved_brightness_pct_ = display_->getBrightnessPercentage();
    }

    buildShell();
    showScreen(Screen::kHome);
    last_status_ms_ = millis();
}

void LauncherUI::onStorageReady(bool sd_ready) {
    (void)sd_ready;
    refreshApps();
    if (radio_ && storage_ && storage_->sdReady()) {
        radio_->tryApplyWifiFromSd();
    }
    refreshStatusBar();
    refreshHomeWidgets();
}

void LauncherUI::refreshApps() {
    if (!registry_ || !storage_) {
        return;
    }
    storage_->updateSdState();
    registry_->begin(storage_->sdReady());
    refreshStatusBar();
}

void LauncherUI::showMessage(const char *title, const char *text) {
    lv_obj_t *mbox;
#if defined(RAKOS_LVGL8)
    mbox = ui_msgbox_create_ok(lv_screen_active(), title, text);
    lv_obj_set_style_bg_color(mbox, ui_color_card(), 0);
    lv_obj_add_event_cb(
        mbox,
        [](lv_event_t *e) {
            if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
                ui_msgbox_close_box(static_cast<lv_obj_t *>(lv_event_get_target(e)));
            }
        },
        LV_EVENT_ALL, nullptr);
#else
    mbox = lv_msgbox_create(lv_screen_active());
    lv_obj_set_style_bg_color(mbox, ui_color_card(), 0);
    lv_msgbox_add_title(mbox, title);
    lv_msgbox_add_text(mbox, text);
    lv_obj_t *ok = lv_msgbox_add_footer_button(mbox, "OK");
    lv_obj_add_event_cb(ok, [](lv_event_t *e) {
        lv_obj_t *btn = static_cast<lv_obj_t *>(lv_event_get_target(e));
        ui_msgbox_close_box(ui_msgbox_get_from_button(btn));
    }, LV_EVENT_CLICKED, nullptr);
#endif
    lv_obj_center(mbox);
    if (display_) {
        display_->forceRefresh();
    }
}

void LauncherUI::setImuSample(const ImuSample &sample) {
    g_last_imu = sample;
}

void LauncherUI::onSdStateChanged() {
    if (radio_ && storage_ && storage_->sdReady()) {
        radio_->tryApplyWifiFromSd();
    } else if (radio_ && storage_ && !storage_->sdReady()) {
        radio_->notifySdUnmounted();
    }
    refreshStatusBar();
    refreshHomeWidgets();
    if (screen_ == Screen::kStatus) {
        refreshStatus();
    } else if (screen_ == Screen::kApps || screen_ == Screen::kHome) {
        showScreen(screen_);
    }
}

void LauncherUI::update() {
    handlePhysicalKeys();

    if (screen_blank_ && display_) {
        int16_t tx = 0;
        int16_t ty = 0;
        if (display_->getTouchCoordinates(tx, ty)) {
            screen_blank_ = false;
            applyBrightnessPct(saved_brightness_pct_);
            refreshStatusBar();
        }
    }

    if (radio_) {
        radio_->update();
    }

    const uint32_t now = millis();

    if ((now - last_status_ms_) >=
#if defined(RAKOS_BOARD_WAVESHARE_LCD5) || defined(RAKOS_BOARD_WAVESHARE_LCD5B)
        2000
#else
        1000
#endif
    ) {
        last_status_ms_ = now;
        refreshStatusBar();
        refreshHomeWidgets();
        if (screen_ == Screen::kStatus) {
            refreshStatus();
        }
    }

    if (screen_ == Screen::kSettings && radio_status_label_ && radio_ &&
        (now - last_radio_ui_ms_) >= 400) {
        last_radio_ui_ms_ = now;
        refreshRadioStatusLabel();
    }
}

void LauncherUI::refreshRadioStatusLabel() {
    if (!radio_status_label_ || !radio_) {
        return;
    }
    String status = radio_->wifiStatusText() + "\n" + radio_->bleStatusText();
    lv_label_set_text(radio_status_label_, status.c_str());
}

void LauncherUI::buildShell() {
    root_ = lv_obj_create(lv_screen_active());
    ui_style_screen(root_);
    lv_obj_set_style_pad_all(root_, UI_SAFE_INSET, 0);
    lv_obj_set_flex_flow(root_, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(root_, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    status_bar_ = lv_obj_create(root_);
    lv_obj_remove_style_all(status_bar_);
    lv_obj_set_width(status_bar_, lv_pct(100));
    lv_obj_set_height(status_bar_, 40);
    lv_obj_set_style_bg_color(status_bar_, ui_color_bg(), 0);
    lv_obj_set_style_bg_opa(status_bar_, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_hor(status_bar_, 8, 0);
    lv_obj_set_flex_flow(status_bar_, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(status_bar_, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *brand = lv_label_create(status_bar_);
    lv_label_set_text(brand, "RAKOS");
    ui_style_accent_label(brand);

    clock_label_ = lv_label_create(status_bar_);
    lv_label_set_text(clock_label_, "");
    ui_style_body_label(clock_label_);
    lv_obj_set_style_text_color(clock_label_, ui_color_muted(), 0);

    battery_label_ = lv_label_create(status_bar_);
    lv_label_set_text(battery_label_, "--");
    ui_style_muted_label(battery_label_);

    content_area_ = lv_obj_create(root_);
    lv_obj_remove_style_all(content_area_);
    lv_obj_set_width(content_area_, lv_pct(100));
    lv_obj_set_flex_grow(content_area_, 1);
    lv_obj_set_style_pad_hor(content_area_, 4, 0);
    lv_obj_set_style_pad_top(content_area_, 8, 0);
    lv_obj_set_style_pad_bottom(content_area_, 8, 0);
    lv_obj_add_flag(content_area_, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(content_area_, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(content_area_, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_flex_flow(content_area_, LV_FLEX_FLOW_COLUMN);

    nav_bar_ = lv_obj_create(root_);
    lv_obj_remove_style_all(nav_bar_);
    lv_obj_set_width(nav_bar_, lv_pct(100));
    lv_obj_set_height(nav_bar_, 52);
    lv_obj_set_style_bg_color(nav_bar_, ui_color_bg(), 0);
    lv_obj_set_style_bg_opa(nav_bar_, LV_OPA_COVER, 0);
    lv_obj_set_style_border_side(nav_bar_, LV_BORDER_SIDE_TOP, 0);
    lv_obj_set_style_border_width(nav_bar_, 1, 0);
    lv_obj_set_style_border_color(nav_bar_, ui_color_card(), 0);
    lv_obj_set_style_pad_hor(nav_bar_, 2, 0);
    lv_obj_set_flex_flow(nav_bar_, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(nav_bar_, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    static const char *kNavLabels[] = {"Home", "Apps", "Setup", "Info"};
    for (int i = 0; i < 4; ++i) {
        lv_obj_t *btn = lv_button_create(nav_bar_);
        nav_btns_[i] = btn;
        lv_obj_set_size(btn, 84, 44);
        lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, 0);
        lv_obj_set_style_bg_opa(btn, LV_OPA_20, LV_STATE_PRESSED);
        lv_obj_set_style_bg_color(btn, ui_color_accent(), LV_STATE_PRESSED);
        lv_obj_set_style_radius(btn, 0, 0);
        lv_obj_set_style_shadow_width(btn, 0, 0);
        lv_obj_add_event_cb(btn, on_nav_clicked, LV_EVENT_CLICKED, reinterpret_cast<void *>(i));

        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, kNavLabels[i]);
        lv_obj_set_style_text_font(lbl, &Inter_20, 0);
        lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_center(lbl);
    }

    radio_keyboard_ = lv_keyboard_create(root_);
    lv_obj_add_flag(radio_keyboard_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_size(radio_keyboard_, lv_pct(100), 150);
    lv_obj_align(radio_keyboard_, LV_ALIGN_BOTTOM_MID, 0, -52);

    refreshStatusBar();
}

void LauncherUI::setNavActive(Screen screen) {
    for (int i = 0; i < 4; ++i) {
        if (!nav_btns_[i]) {
            continue;
        }
        const bool active = kNavScreens[i] == screen;
        lv_obj_set_style_border_side(nav_btns_[i], LV_BORDER_SIDE_TOP, 0);
        lv_obj_set_style_border_width(nav_btns_[i], active ? 2 : 0, 0);
        lv_obj_set_style_border_color(nav_btns_[i], ui_color_accent(), 0);

        lv_obj_t *lbl = lv_obj_get_child(nav_btns_[i], 0);
        if (lbl) {
            lv_obj_set_style_text_color(lbl, active ? ui_color_accent() : ui_color_muted(), 0);
        }
    }
}

void LauncherUI::showScreen(Screen screen) {
    screen_ = screen;
    status_label_ = nullptr;
    home_time_label_ = nullptr;
    home_wifi_label_ = nullptr;
    brightness_slider_ = nullptr;
    volume_slider_ = nullptr;
    wifi_switch_ = nullptr;
    ble_switch_ = nullptr;
    wifi_ssid_ta_ = nullptr;
    wifi_pass_ta_ = nullptr;
    ble_name_ta_ = nullptr;
    radio_status_label_ = nullptr;

    if (content_area_) {
        lv_obj_clean(content_area_);
        lv_obj_scroll_to_y(content_area_, 0, LV_ANIM_OFF);
    }

    setNavActive(screen_);

    switch (screen_) {
        case Screen::kHome:
            buildHome();
            break;
        case Screen::kApps:
#if defined(RAKOS_UI_APP_GRID)
            buildAppGridPage();
#else
            buildApps();
#endif
            break;
        case Screen::kSettings:
            buildSettings();
            last_radio_ui_ms_ = 0;
            break;
        case Screen::kStatus:
            buildStatus();
            break;
    }
}

void LauncherUI::refreshStatusBar() {
    if (!battery_label_) {
        return;
    }

    if (clock_label_ && radio_) {
        const String t = radio_->localTimeText();
        if (!t.isEmpty()) {
            lv_label_set_text(clock_label_, t.c_str());
            lv_obj_set_style_text_color(clock_label_, ui_color_ok(), 0);
        } else if (radio_->wifiState() == WifiLinkState::kScanning) {
            lv_label_set_text(clock_label_, "Scan");
            lv_obj_set_style_text_color(clock_label_, ui_color_muted(), 0);
        } else if (radio_->wifiState() == WifiLinkState::kConnecting) {
            lv_label_set_text(clock_label_, "...");
            lv_obj_set_style_text_color(clock_label_, ui_color_muted(), 0);
        } else {
            lv_label_set_text(clock_label_, "");
        }
    }

    String text;
    if (screen_blank_) {
        text = "Screen off";
    } else {
        text = "RAKOS";
    }

    if (storage_) {
        text += storage_->sdReady() ? "  SD" : "  !SD";
    }

    if (radio_) {
        if (radio_->wifiState() == WifiLinkState::kConnected) {
            text += "  WiFi";
        }
        if (radio_->bleActive()) {
            text += "  BLE";
        }
    }

    lv_label_set_text(battery_label_, text.c_str());
    lv_obj_set_style_text_color(battery_label_,
                                (storage_ && storage_->sdReady()) ? ui_color_ok() : ui_color_muted(), 0);
}

void LauncherUI::refreshHomeWidgets() {
    if (screen_ != Screen::kHome) {
        return;
    }

    if (home_time_label_ && radio_) {
        const String t = radio_->localTimeText();
        if (!t.isEmpty()) {
            struct tm ti;
            if (getLocalTime(&ti)) {
                char buf[32];
                strftime(buf, sizeof(buf), "%Y-%m-%d  %H:%M:%S", &ti);
                lv_label_set_text(home_time_label_, buf);
                lv_obj_set_style_text_color(home_time_label_, ui_color_ok(), 0);
            }
        } else {
            lv_label_set_text(home_time_label_, "--:--:--");
            lv_obj_set_style_text_color(home_time_label_, ui_color_muted(), 0);
        }
    }

    if (home_wifi_label_ && radio_) {
        String wifi;
        switch (radio_->wifiState()) {
            case WifiLinkState::kScanning:
                wifi = "WiFi: scanning...";
                break;
            case WifiLinkState::kConnecting:
                wifi = "WiFi: connecting...";
                break;
            case WifiLinkState::kConnected:
                wifi = "WiFi: " + radio_->ipAddress();
                break;
            case WifiLinkState::kFailed:
                wifi = "WiFi: failed (check wifi.ini on SD)";
                break;
            default:
                if (!radio_->config().wifi_enabled) {
                    wifi = "WiFi: add wifi.ini to SD root, or Setup";
                } else {
                    wifi = radio_->wifiStatusText();
                }
                break;
        }
        lv_label_set_text(home_wifi_label_, wifi.c_str());
    }
}

void LauncherUI::applyBrightnessPct(uint8_t pct) {
    if (!display_) {
        return;
    }
    if (pct > 100) {
        pct = 100;
    }
    saved_brightness_pct_ = pct;
    if (screen_blank_) {
        return;
    }
    const uint8_t level = (uint8_t)(((uint16_t)pct * 255 + 50) / 100);
    display_->setBrightness(level);
}

void LauncherUI::toggleScreenBlank() {
    if (!display_) {
        return;
    }

    if (screen_blank_) {
        screen_blank_ = false;
        applyBrightnessPct(saved_brightness_pct_);
    } else {
        saved_brightness_pct_ = display_->getBrightnessPercentage();
        screen_blank_ = true;
        display_->setBrightness(0, false);
    }
    refreshStatusBar();
}

void LauncherUI::showShutdownDialog() {
    lv_obj_t *mbox;
#if defined(RAKOS_LVGL8)
    mbox = ui_msgbox_create_yes_no(lv_screen_active(), "Power Off",
                                   "Shut down the device?", "Power Off", "Cancel");
    lv_obj_set_style_bg_color(mbox, ui_color_card(), 0);
    lv_obj_add_event_cb(mbox, on_shutdown_confirm, LV_EVENT_VALUE_CHANGED, nullptr);
#else
    mbox = lv_msgbox_create(lv_screen_active());
    lv_obj_set_style_bg_color(mbox, ui_color_card(), 0);
    lv_msgbox_add_title(mbox, "Power Off");
    lv_msgbox_add_text(mbox, "Shut down the device?\nHold PWR 4s also powers off via PMIC.");
    lv_obj_t *yes = lv_msgbox_add_footer_button(mbox, "Power Off");
    lv_obj_t *no = lv_msgbox_add_footer_button(mbox, "Cancel");
    lv_obj_set_style_bg_color(yes, ui_color_danger(), 0);
    lv_obj_add_event_cb(yes, on_shutdown_confirm, LV_EVENT_CLICKED, reinterpret_cast<void *>(1));
    lv_obj_add_event_cb(no, on_shutdown_confirm, LV_EVENT_CLICKED, reinterpret_cast<void *>(0));
#endif
    lv_obj_center(mbox);
}

void LauncherUI::playFeedback(uint16_t freq_hz) {
    if (audio_ && audio_->ready()) {
        audio_->beep(freq_hz, 42);
    }
}

void LauncherUI::handlePhysicalKeys() {
    if (!input_) {
        return;
    }

    if (input_->bootPressedEdge()) {
        playFeedback(740);
        showScreen(Screen::kHome);
    }

    if (input_->bootLongPress(1500)) {
        playFeedback(520);
        BootManager::reboot();
    }

    if (input_->pwrPressedEdge()) {
        playFeedback(880);
        toggleScreenBlank();
    }

    if (input_->pwrLongPress(2000)) {
        playFeedback(440);
        showShutdownDialog();
    }
}

void LauncherUI::buildHome() {
    lv_obj_t *wrap = lv_obj_create(content_area_);
    ui_style_page(wrap);
    lv_obj_set_style_pad_row(wrap, 12, 0);

    lv_obj_t *title = lv_label_create(wrap);
    lv_label_set_text(title, "RAKOS");
    ui_style_title_label(title);

    home_time_label_ = lv_label_create(wrap);
    lv_label_set_text(home_time_label_, "--:--:--");
    ui_style_accent_label(home_time_label_);
    lv_obj_set_style_text_font(home_time_label_, &Inter_30, 0);

    home_wifi_label_ = lv_label_create(wrap);
    lv_label_set_text(home_wifi_label_, "WiFi: --");
    ui_style_muted_label(home_wifi_label_);
    lv_label_set_long_mode(home_wifi_label_, LV_LABEL_LONG_DOT);
    lv_obj_set_width(home_wifi_label_, lv_pct(100));

    const size_t app_count = registry_ ? registry_->apps().size() : 0;
    const bool sd_ok = storage_ && storage_->sdReady();

    lv_obj_t *sub = lv_label_create(wrap);
    String sub_text = sd_ok ? String(app_count) + " app(s) on SD" : "SD not mounted";
    lv_label_set_text(sub, sub_text.c_str());
    ui_style_muted_label(sub);

    ui_make_primary_btn(wrap, "Open Applications", on_home_apps);

    if (sd_ok && app_count == 0) {
        lv_obj_t *hint = lv_obj_create(wrap);
        ui_style_card_subtle(hint);
        lv_obj_set_width(hint, lv_pct(100));
        lv_obj_t *hint_lbl = lv_label_create(hint);
        lv_label_set_text(hint_lbl,
                          "Copy to SD card root (PC):\n"
                          "Games/Clock/app.bin\n"
                          "wifi.ini (ssid/password)");
        ui_style_muted_label(hint_lbl);
    }

    refreshHomeWidgets();
}

void LauncherUI::buildApps() {
    lv_obj_t *wrap = lv_obj_create(content_area_);
    ui_style_page(wrap);
    lv_obj_set_style_pad_row(wrap, 10, 0);
    lv_obj_set_height(wrap, lv_pct(100));
    lv_obj_set_flex_grow(wrap, 1);

    lv_obj_t *title = lv_label_create(wrap);
    lv_label_set_text(title, "Applications");
    ui_style_title_label(title);

    ui_make_outline_btn(wrap, LV_SYMBOL_REFRESH " Refresh SD apps", on_apps_refresh);

    lv_obj_t *list = lv_list_create(wrap);
    lv_obj_set_width(list, lv_pct(100));
    lv_obj_set_flex_grow(list, 1);
    lv_obj_add_flag(list, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(list, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_style_bg_color(list, ui_color_card(), 0);
    lv_obj_set_style_bg_opa(list, LV_OPA_40, 0);
    lv_obj_set_style_radius(list, 16, 0);
    lv_obj_set_style_border_width(list, 1, 0);
    lv_obj_set_style_border_color(list, ui_color_muted(), 0);
    lv_obj_set_style_border_opa(list, LV_OPA_20, 0);

    const bool ota1_ready = BootManager::ota1HasFirmware();
    if (ota1_ready) {
        lv_obj_t *usb_btn = lv_list_add_button(list, LV_SYMBOL_PLAY, "USB / ota_1 (flashed)");
        lv_obj_add_event_cb(usb_btn, on_home_run_app, LV_EVENT_CLICKED, nullptr);
    }

    if (!storage_ || !storage_->sdReady()) {
        lv_list_add_text(list, "SD card not mounted");
        lv_list_add_text(list, "Insert TF card, then Refresh");
        lv_list_add_text(list, "Expected on PC (SD root):");
        lv_list_add_text(list, "Games/Clock/app.bin");
    } else if (registry_->apps().empty()) {
        lv_list_add_text(list, "No apps found on SD");
        lv_list_add_text(list, "Put on SD root (not in sdcard/):");
        lv_list_add_text(list, "Games/Clock/app.bin");
    } else {
        for (size_t i = 0; i < registry_->apps().size(); ++i) {
            const AppEntry &app = registry_->apps()[i];
            String line = app.category + " / " + app.name;
            if (!app.has_bin) {
                line += " (no bin)";
            }
            const char *icon = app.has_bin ? LV_SYMBOL_PLAY : LV_SYMBOL_CLOSE;
            lv_obj_t *btn = lv_list_add_button(list, icon, line.c_str());
            if (app.has_bin) {
                lv_obj_add_event_cb(btn, on_app_selected, LV_EVENT_CLICKED, reinterpret_cast<void *>(i));
            }
        }
    }

    if (!ota1_ready) {
        lv_list_add_text(list, "No USB app in ota_1");
        lv_list_add_text(list, "pio run -e hello_app -t upload");
    }
}

void LauncherUI::buildSettings() {
    lv_obj_t *wrap = lv_obj_create(content_area_);
    ui_style_page(wrap);
    lv_obj_set_style_pad_row(wrap, 12, 0);

    lv_obj_t *title = lv_label_create(wrap);
    lv_label_set_text(title, "Settings");
    ui_style_title_label(title);

    lv_obj_t *bri_card = make_setting_row(wrap, "Brightness",
#if defined(RAKOS_BOARD_WAVESHARE_LCD5) || defined(RAKOS_BOARD_WAVESHARE_LCD5B)
                                          "LCD backlight (CH422G EXIO2)");
#else
                                          "Slider or PWR to toggle screen");
#endif
    brightness_slider_ = lv_slider_create(bri_card);
    lv_obj_set_width(brightness_slider_, lv_pct(100));
    lv_slider_set_range(brightness_slider_, 10, 100);
    lv_slider_set_value(brightness_slider_, display_ ? display_->getBrightnessPercentage() : 80, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(brightness_slider_, ui_color_card(), LV_PART_MAIN);
    lv_obj_set_style_bg_color(brightness_slider_, ui_color_accent(), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(brightness_slider_, ui_color_accent(), LV_PART_KNOB);
    lv_obj_add_event_cb(brightness_slider_, on_brightness_changed, LV_EVENT_VALUE_CHANGED, nullptr);

    lv_obj_t *vol_card = make_setting_row(wrap, "Volume",
#if defined(RAKOS_BOARD_WAVESHARE_LCD5) || defined(RAKOS_BOARD_WAVESHARE_LCD5B)
                                          "UI beep on isolated DO0");
#else
                                          "UI beep / speaker level");
#endif
    volume_slider_ = lv_slider_create(vol_card);
    lv_obj_set_width(volume_slider_, lv_pct(100));
    lv_slider_set_range(volume_slider_, 0, 100);
    lv_slider_set_value(volume_slider_, audio_ ? audio_->volumePercent() : 75, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(volume_slider_, ui_color_card(), LV_PART_MAIN);
    lv_obj_set_style_bg_color(volume_slider_, ui_color_accent(), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(volume_slider_, ui_color_accent(), LV_PART_KNOB);
    lv_obj_add_event_cb(volume_slider_, on_volume_changed, LV_EVENT_VALUE_CHANGED, nullptr);

    const RadioConfig rc = radio_ ? radio_->config() : RadioConfig{};

    lv_obj_t *wifi_card = make_setting_row(wrap, "WiFi", "SSID/password in NVS; also /sdcard/wifi.ini on boot");
    lv_obj_t *wifi_sw_row = lv_obj_create(wifi_card);
    lv_obj_remove_style_all(wifi_sw_row);
    lv_obj_set_width(wifi_sw_row, lv_pct(100));
    lv_obj_set_height(wifi_sw_row, 36);
    lv_obj_set_flex_flow(wifi_sw_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(wifi_sw_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_t *wifi_sw_lbl = lv_label_create(wifi_sw_row);
    lv_label_set_text(wifi_sw_lbl, "Enable WiFi");
    ui_style_body_label(wifi_sw_lbl);
    wifi_switch_ = lv_switch_create(wifi_sw_row);
    if (rc.wifi_enabled) {
        lv_obj_add_state(wifi_switch_, LV_STATE_CHECKED);
    }

    wifi_ssid_ta_ = lv_textarea_create(wifi_card);
    lv_obj_set_width(wifi_ssid_ta_, lv_pct(100));
    lv_textarea_set_one_line(wifi_ssid_ta_, true);
    lv_textarea_set_max_length(wifi_ssid_ta_, 32);
    lv_textarea_set_placeholder_text(wifi_ssid_ta_, "WiFi SSID");
    lv_textarea_set_text(wifi_ssid_ta_, rc.wifi_ssid);
    lv_obj_add_event_cb(wifi_ssid_ta_, on_radio_ta_focus, LV_EVENT_FOCUSED, nullptr);
    lv_obj_add_event_cb(wifi_ssid_ta_, on_radio_ta_focus, LV_EVENT_DEFOCUSED, nullptr);

    wifi_pass_ta_ = lv_textarea_create(wifi_card);
    lv_obj_set_width(wifi_pass_ta_, lv_pct(100));
    lv_textarea_set_one_line(wifi_pass_ta_, true);
    lv_textarea_set_max_length(wifi_pass_ta_, 64);
    lv_textarea_set_password_mode(wifi_pass_ta_, true);
    lv_textarea_set_placeholder_text(wifi_pass_ta_, "Password");
    lv_textarea_set_text(wifi_pass_ta_, rc.wifi_pass);
    lv_obj_add_event_cb(wifi_pass_ta_, on_radio_ta_focus, LV_EVENT_FOCUSED, nullptr);
    lv_obj_add_event_cb(wifi_pass_ta_, on_radio_ta_focus, LV_EVENT_DEFOCUSED, nullptr);

    lv_obj_t *ble_card = make_setting_row(wrap, "Bluetooth LE", "Advertise device name");
    lv_obj_t *ble_sw_row = lv_obj_create(ble_card);
    lv_obj_remove_style_all(ble_sw_row);
    lv_obj_set_width(ble_sw_row, lv_pct(100));
    lv_obj_set_height(ble_sw_row, 36);
    lv_obj_set_flex_flow(ble_sw_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(ble_sw_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_t *ble_sw_lbl = lv_label_create(ble_sw_row);
    lv_label_set_text(ble_sw_lbl, "Enable BLE");
    ui_style_body_label(ble_sw_lbl);
    ble_switch_ = lv_switch_create(ble_sw_row);
    if (rc.ble_enabled) {
        lv_obj_add_state(ble_switch_, LV_STATE_CHECKED);
    }

    ble_name_ta_ = lv_textarea_create(ble_card);
    lv_obj_set_width(ble_name_ta_, lv_pct(100));
    lv_textarea_set_one_line(ble_name_ta_, true);
    lv_textarea_set_max_length(ble_name_ta_, 20);
    lv_textarea_set_placeholder_text(ble_name_ta_, "BLE name");
    lv_textarea_set_text(ble_name_ta_, rc.ble_name);
    lv_obj_add_event_cb(ble_name_ta_, on_radio_ta_focus, LV_EVENT_FOCUSED, nullptr);
    lv_obj_add_event_cb(ble_name_ta_, on_radio_ta_focus, LV_EVENT_DEFOCUSED, nullptr);

    radio_status_label_ = lv_label_create(wrap);
    refreshRadioStatusLabel();
    ui_style_muted_label(radio_status_label_);

    ui_make_primary_btn(wrap, "Save WiFi / BLE", on_radio_save);

    const bool managed = config_->data().mode == OsMode::kManaged;
    lv_obj_t *mode_card = make_setting_row(wrap, "OS Mode", managed ? "Managed" : "Maker");
    ui_make_primary_btn(mode_card, managed ? "Switch to Maker" : "Switch to Managed", on_settings_mode);

    const bool boot_os = config_->data().default_boot_subtype == ESP_PARTITION_SUBTYPE_APP_OTA_0;
    lv_obj_t *boot_card = make_setting_row(wrap, "Default Boot", boot_os ? "OS (ota_0)" : "App (ota_1)");

    lv_obj_t *boot_row = lv_obj_create(boot_card);
    lv_obj_remove_style_all(boot_row);
    lv_obj_set_width(boot_row, lv_pct(100));
    lv_obj_set_height(boot_row, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(boot_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(boot_row, 8, 0);

    lv_obj_t *os_btn = ui_make_outline_btn(boot_row, "OS", [](lv_event_t *e) {
        if (!g_ui || !g_ui->config_) {
            return;
        }
        g_ui->config_->setDefaultBootSubtype(ESP_PARTITION_SUBTYPE_APP_OTA_0);
        g_ui->config_->save();
        g_ui->showScreen(Screen::kSettings);
    });
    lv_obj_set_flex_grow(os_btn, 1);

    lv_obj_t *app_btn = ui_make_outline_btn(boot_row, "App", [](lv_event_t *e) {
        if (!g_ui || !g_ui->config_) {
            return;
        }
        g_ui->config_->setDefaultBootSubtype(ESP_PARTITION_SUBTYPE_APP_OTA_1);
        g_ui->config_->save();
        g_ui->showScreen(Screen::kSettings);
    });
    lv_obj_set_flex_grow(app_btn, 1);

    lv_obj_t *pwr_card = make_setting_row(wrap, "Power", "AXP2101 shutdown");
    lv_obj_t *off_btn = lv_button_create(pwr_card);
    lv_obj_set_width(off_btn, lv_pct(100));
    lv_obj_set_height(off_btn, 44);
    lv_obj_set_style_bg_color(off_btn, ui_color_danger(), 0);
    lv_obj_set_style_radius(off_btn, 22, 0);
    lv_obj_add_event_cb(off_btn, on_shutdown_btn, LV_EVENT_CLICKED, nullptr);
    lv_obj_t *off_lbl = lv_label_create(off_btn);
    lv_label_set_text(off_lbl, "Shut Down");
    lv_obj_set_style_text_font(off_lbl, &Inter_20, 0);
    lv_obj_center(off_lbl);
}

void LauncherUI::buildStatus() {
    lv_obj_t *wrap = lv_obj_create(content_area_);
    ui_style_page(wrap);

    lv_obj_t *title = lv_label_create(wrap);
    lv_label_set_text(title, "System Info");
    ui_style_title_label(title);

    status_label_ = lv_label_create(wrap);
    lv_label_set_long_mode(status_label_, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(status_label_, lv_pct(100));
    ui_style_muted_label(status_label_);
    refreshStatus();
}

void LauncherUI::refreshStatus() {
    if (!status_label_) {
        return;
    }

    const PartitionSummary s = BootManager::getSummary();
    String text;
    text += BootManager::isRunningOsSlot() ? "Slot: ota_0 (OS)\n" : "Slot: ota_1 (App)\n";
    text += "Boots: " + String(config_->data().boot_counter) + "\n";
    text += "SD: " + String(storage_ && storage_->sdReady() ? "OK" : "N/A") + "\n";
    if (storage_ && storage_->sdReady()) {
        text += "SD size: " + String(storage_->sdSizeMb()) + " MB\n";
    }
    text += "SPIFFS: " + String(storage_ && storage_->spiffsReady() ? "OK" : "N/A") + "\n";
    text += "Apps: " + String(registry_ ? registry_->apps().size() : 0) + "\n";
    if (registry_ && !registry_->lastScanInfo().isEmpty()) {
        text += "Scan: " + registry_->lastScanInfo() + "\n";
    }
    text += "Brightness: " + String(display_ ? display_->getBrightnessPercentage() : 0) + "%\n";

    if (radio_) {
        text += "\nRadio\n";
        text += "WiFi: " + radio_->wifiStatusText() + "\n";
        text += "BLE: " + radio_->bleStatusText() + "\n";
    }

#if defined(RAKOS_BOARD_WAVESHARE_LCD5) || defined(RAKOS_BOARD_WAVESHARE_LCD5B)
    text += "\nBoard I/O\n";
    text += "RS485: GPIO" + String(RS485_RX_PIN) + "/" + String(RS485_TX_PIN) + "\n";
    text += "CAN: GPIO" + String(CAN_TX_PIN) + "/" + String(CAN_RX_PIN) + " (TWAI)\n";
    text += "Isolated DO: EXIO" + String(EXPIO_DO0) + "/" + String(EXPIO_DO1) + "\n";
    text += "Beep: " + String(audio_ && audio_->ready() ? "DO0 OK" : "N/A");
    if (audio_ && audio_->ready()) {
        text += " (" + String(audio_->volumePercent()) + "%)";
    }
    text += "\n";
    text += "RTC: PCF85063A @ I2C\n";
#else
    if (power_ && power_->ready()) {
        const PowerStatus &p = power_->status();
        text += "\nAXP2101\n";
        text += "Battery: ";
        text += p.battery_connected ? String(p.battery_percent) + "% (" + String(p.battery_mv) + " mV)\n"
                                    : "N/A\n";
        text += "Charge: " + String(p.charging ? "yes" : "no") + "\n";
        text += "VBUS: " + String(p.vbus_in ? "in" : "out") + " " + String(p.vbus_mv) + " mV\n";
        text += "VSYS: " + String(p.system_mv) + " mV\n";
    } else {
        text += "\nAXP2101: N/A\n";
    }

    if (imu_ && imu_->ready()) {
        text += "\nQMI8658\n";
        if (g_last_imu.accel_valid) {
            text += "Accel: " + String(g_last_imu.ax, 2) + ", " + String(g_last_imu.ay, 2) + ", " +
                    String(g_last_imu.az, 2) + "\n";
        }
        if (g_last_imu.gyro_valid) {
            text += "Gyro: " + String(g_last_imu.gx, 2) + ", " + String(g_last_imu.gy, 2) + ", " +
                    String(g_last_imu.gz, 2) + "\n";
        }
    } else {
        text += "\nQMI8658: N/A\n";
    }

    text += "\nES8311: " + String(audio_ && audio_->ready() ? "OK" : "N/A");
    if (audio_ && audio_->ready()) {
        text += " (" + String(audio_->volumePercent()) + "%)";
    }
    text += "\n";
#endif

    if (s.ota0) {
        text += "ota_0 @ 0x" + String(s.ota0->address, HEX) + "\n";
    }
    if (s.ota1) {
        text += "ota_1 @ 0x" + String(s.ota1->address, HEX) + "\n";
    }
    lv_label_set_text(status_label_, text.c_str());
}

void LauncherUI::pumpUi() {
#if defined(RAKOS_BOARD_WAVESHARE_LCD5) || defined(RAKOS_BOARD_WAVESHARE_LCD5B)
    // RGB panel: lvgl_port_task owns lv_timer_handler(); avoid double refresh.
#else
    lv_timer_handler();
    if (display_) {
        display_->forceRefresh();
    }
#endif
}

void LauncherUI::showInstallOverlay(const AppEntry &app) {
    hideInstallOverlay();

    install_overlay_ = lv_obj_create(lv_layer_top());
    lv_obj_remove_style_all(install_overlay_);
    lv_obj_set_size(install_overlay_, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(install_overlay_, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(install_overlay_, LV_OPA_80, 0);
    lv_obj_clear_flag(install_overlay_, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *panel = lv_obj_create(install_overlay_);
    ui_style_card(panel);
    lv_obj_set_width(panel, lv_pct(88));
    lv_obj_set_height(panel, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(panel, 10, 0);
    lv_obj_center(panel);

    install_title_ = lv_label_create(panel);
    String title = "Install: " + app.category + " / " + app.name;
    lv_label_set_text(install_title_, title.c_str());
    ui_style_title_label(install_title_);
    lv_obj_set_width(install_title_, lv_pct(100));

    lv_obj_t *hint = lv_label_create(panel);
    lv_label_set_text(hint, "SD card -> flash (ota_1)\nPlease wait...");
    ui_style_muted_label(hint);
    lv_label_set_long_mode(hint, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(hint, lv_pct(100));

    install_phase_ = lv_label_create(panel);
    lv_label_set_text(install_phase_, "Preparing...");
    ui_style_body_label(install_phase_);
    lv_obj_set_width(install_phase_, lv_pct(100));

    install_bar_ = lv_bar_create(panel);
    lv_obj_set_width(install_bar_, lv_pct(100));
    lv_obj_set_height(install_bar_, 12);
    lv_bar_set_range(install_bar_, 0, 100);
    lv_bar_set_value(install_bar_, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(install_bar_, ui_color_card(), LV_PART_MAIN);
    lv_obj_set_style_bg_color(install_bar_, ui_color_accent(), LV_PART_INDICATOR);

    install_pct_ = lv_label_create(panel);
    lv_label_set_text(install_pct_, "0%");
    ui_style_accent_label(install_pct_);

    pumpUi();
}

void LauncherUI::updateInstallOverlay(const char *phase, size_t done, size_t total) {
    if (install_phase_ && phase) {
        lv_label_set_text(install_phase_, phase);
    }

    if (install_bar_ && install_pct_) {
        int pct = 0;
        if (total > 0) {
            pct = static_cast<int>((static_cast<uint64_t>(done) * 100ULL) / static_cast<uint64_t>(total));
            if (pct > 100) {
                pct = 100;
            }
        }
        lv_bar_set_value(install_bar_, pct, LV_ANIM_OFF);
        lv_label_set_text(install_pct_, (String(pct) + "%").c_str());
    }

    pumpUi();
}

void LauncherUI::hideInstallOverlay() {
    if (install_overlay_) {
        lv_obj_delete(install_overlay_);
        install_overlay_ = nullptr;
        install_title_ = nullptr;
        install_phase_ = nullptr;
        install_bar_ = nullptr;
        install_pct_ = nullptr;
        pumpUi();
    }
}

namespace {

File openSdAppBin(const String &path) {
    fs::FS &fs = rakos::sdFs();
    File f = fs.open(path.c_str(), FILE_READ);
    if (f) {
        return f;
    }
    if (!path.startsWith(SD_MOUNT_POINT)) {
        const String alt = String(SD_MOUNT_POINT) + (path.startsWith("/") ? path : String("/") + path);
        f = fs.open(alt.c_str(), FILE_READ);
        if (f) {
            return f;
        }
    }
    if (path.startsWith(SD_MOUNT_POINT)) {
        const String alt = path.substring(strlen(SD_MOUNT_POINT));
        f = fs.open(alt.c_str(), FILE_READ);
    }
    return f;
}

} // namespace

bool LauncherUI::flashAppBinToOta1(const String &path) {
    const esp_partition_t *part =
        esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_1, nullptr);
    if (!part) {
        return false;
    }

    File f = openSdAppBin(path);
    if (!f) {
        Serial.printf("[APP] Cannot open SD file: %s\n", path.c_str());
        return false;
    }

    const size_t image_size = f.size();
    if (image_size == 0 || image_size > part->size) {
        f.close();
        return false;
    }

    constexpr size_t kEraseStep = 0x10000;
    size_t erased = 0;
    while (erased < part->size) {
        const size_t chunk = (part->size - erased) > kEraseStep ? kEraseStep : (part->size - erased);
        updateInstallOverlay("Erasing ota_1...", erased, part->size);
        if (esp_partition_erase_range(part, erased, chunk) != ESP_OK) {
            f.close();
            return false;
        }
        erased += chunk;
    }

    uint8_t buf[4096];
    size_t offset = 0;
    size_t last_ui = 0;
    while (f.available()) {
        const size_t n = f.read(buf, sizeof(buf));
        if (n == 0) {
            break;
        }
        if (esp_partition_write(part, offset, buf, n) != ESP_OK) {
            f.close();
            return false;
        }
        offset += n;
        if ((offset - last_ui) >= 32768 || offset == image_size) {
            updateInstallOverlay("Copying SD -> flash...", offset, image_size);
            last_ui = offset;
        }
    }
    f.close();

    if (offset != image_size) {
        return false;
    }

    updateInstallOverlay("Install complete", image_size, image_size);
    return true;
}

void LauncherUI::launchSelectedApp(const AppEntry &app) {
    if (!app.has_bin) {
        showMessage("Cannot launch", "app.bin not found on SD");
        return;
    }

    playFeedback(920);
    showInstallOverlay(app);

    Serial.printf("[APP] Flashing %s\n", app.bin_path.c_str());

    if (!flashAppBinToOta1(app.bin_path)) {
        Serial.println("[APP] Flash failed");
        hideInstallOverlay();
        showMessage("Flash failed", "Check SD file and size");
        return;
    }

    Serial.println("[APP] Flash OK, rebooting to app");
    updateInstallOverlay("Rebooting to app...", 100, 100);
    delay(400);

    if (BootManager::setNextBootApp()) {
        BootManager::reboot();
    } else {
        hideInstallOverlay();
        showMessage("Boot error", "Could not set ota_1 boot slot");
    }
}

void LauncherUI::on_nav_clicked(lv_event_t *e) {
    if (!g_ui) {
        return;
    }
    const int idx = static_cast<int>(reinterpret_cast<uintptr_t>(lv_event_get_user_data(e)));
    if (idx >= 0 && idx < 4) {
        g_ui->playFeedback(920);
        g_ui->showScreen(kNavScreens[idx]);
    }
}

void LauncherUI::on_home_apps(lv_event_t *e) {
    if (g_ui) {
        g_ui->showScreen(Screen::kApps);
    }
}

void LauncherUI::on_home_run_app(lv_event_t *e) {
    if (BootManager::setNextBootApp()) {
        BootManager::reboot();
    }
}

void LauncherUI::on_apps_refresh(lv_event_t *e) {
    if (!g_ui) {
        return;
    }
    g_ui->playFeedback(920);
    g_ui->refreshApps();

    if (g_ui->storage_ && !g_ui->storage_->sdReady()) {
        g_ui->showMessage("SD not mounted", "Insert TF card and tap Refresh again.\nCheck Status page for SD: OK.");
    } else if (g_ui->registry_ && g_ui->registry_->apps().empty()) {
        String msg = "On PC, copy to SD card ROOT:\n"
                     "Games/Clock/app.bin\n\n";
        if (!g_ui->registry_->lastScanInfo().isEmpty()) {
            msg += "Scan: " + g_ui->registry_->lastScanInfo();
        }
        g_ui->showMessage("No apps found", msg.c_str());
    }

    g_ui->showScreen(Screen::kApps);
}

void LauncherUI::on_app_selected(lv_event_t *e) {
    if (!g_ui || !g_ui->registry_) {
        return;
    }
    const size_t idx = reinterpret_cast<size_t>(lv_event_get_user_data(e));
    if (idx >= g_ui->registry_->apps().size()) {
        return;
    }
    g_ui->launchSelectedApp(g_ui->registry_->apps()[idx]);
}

void LauncherUI::on_settings_mode(lv_event_t *e) {
    if (!g_ui || !g_ui->config_) {
        return;
    }
    const OsMode next = g_ui->config_->data().mode == OsMode::kManaged ? OsMode::kMaker : OsMode::kManaged;
    g_ui->config_->setMode(next);
    g_ui->config_->save();
    g_ui->showScreen(Screen::kSettings);
}

void LauncherUI::on_brightness_changed(lv_event_t *e) {
    if (!g_ui || !g_ui->display_) {
        return;
    }
    lv_obj_t *slider = static_cast<lv_obj_t *>(lv_event_get_target(e));
    const int val = lv_slider_get_value(slider);
    g_ui->screen_blank_ = false;
    g_ui->applyBrightnessPct(static_cast<uint8_t>(val));
    g_ui->refreshStatusBar();
}

void LauncherUI::on_volume_changed(lv_event_t *e) {
    if (!g_ui || !g_ui->audio_) {
        return;
    }
    lv_obj_t *slider = static_cast<lv_obj_t *>(lv_event_get_target(e));
    const uint8_t val = static_cast<uint8_t>(lv_slider_get_value(slider));
    g_ui->audio_->setVolume(val);
    if (g_ui->audio_->ready() && val > 0) {
        g_ui->audio_->beep(1040, 48);
    }
}

void LauncherUI::on_radio_save(lv_event_t *e) {
    (void)e;
    if (!g_ui || !g_ui->radio_) {
        return;
    }

    g_ui->playFeedback(920);

    const bool wifi_on = g_ui->wifi_switch_ && lv_obj_has_state(g_ui->wifi_switch_, LV_STATE_CHECKED);
    const bool ble_on = g_ui->ble_switch_ && lv_obj_has_state(g_ui->ble_switch_, LV_STATE_CHECKED);
    const char *ssid = g_ui->wifi_ssid_ta_ ? lv_textarea_get_text(g_ui->wifi_ssid_ta_) : "";
    const char *pass = g_ui->wifi_pass_ta_ ? lv_textarea_get_text(g_ui->wifi_pass_ta_) : "";
    const char *ble = g_ui->ble_name_ta_ ? lv_textarea_get_text(g_ui->ble_name_ta_) : "RAKOS";

    g_ui->radio_->setWifiEnabled(wifi_on);
    g_ui->radio_->setBleEnabled(ble_on);
    g_ui->radio_->setWifiCredentials(ssid, pass);
    g_ui->radio_->setBleName(ble);
    g_ui->radio_->apply();

    if (g_ui->storage_ && g_ui->storage_->sdReady() && wifi_on && ssid && ssid[0]) {
        saveWifiToSd(ssid, pass ? pass : "");
    }

    g_ui->refreshRadioStatusLabel();
    g_ui->refreshStatusBar();
    String saved = g_ui->radio_->wifiStatusText() + "\n" + g_ui->radio_->bleStatusText();
    g_ui->showMessage("Radio saved", saved.c_str());
    if (g_ui->display_) {
        g_ui->display_->forceRefresh();
    }
}

void LauncherUI::on_radio_ta_focus(lv_event_t *e) {
    if (!g_ui || !g_ui->radio_keyboard_) {
        return;
    }
    lv_obj_t *ta = static_cast<lv_obj_t *>(lv_event_get_target(e));
    if (lv_event_get_code(e) == LV_EVENT_FOCUSED) {
        lv_keyboard_set_textarea(g_ui->radio_keyboard_, ta);
        lv_obj_clear_flag(g_ui->radio_keyboard_, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(g_ui->radio_keyboard_, LV_OBJ_FLAG_HIDDEN);
        lv_keyboard_set_textarea(g_ui->radio_keyboard_, nullptr);
    }
}

void LauncherUI::on_shutdown_btn(lv_event_t *e) {
    if (g_ui) {
        g_ui->showShutdownDialog();
    }
}

void LauncherUI::on_shutdown_confirm(lv_event_t *e) {
#if defined(RAKOS_LVGL8)
    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) {
        return;
    }
    lv_obj_t *mbox = static_cast<lv_obj_t *>(lv_event_get_target(e));
    const uint16_t btn_id = lv_msgbox_get_active_btn(mbox);
    ui_msgbox_close_box(mbox);
    if (btn_id == 0 && g_ui && g_ui->power_) {
        g_ui->power_->shutdown();
    }
#else
    const bool confirm = reinterpret_cast<uintptr_t>(lv_event_get_user_data(e)) != 0;
    lv_obj_t *mbox = ui_msgbox_get_from_button(static_cast<lv_obj_t *>(lv_event_get_target(e)));
    ui_msgbox_close_box(mbox);

    if (confirm && g_ui && g_ui->power_) {
        g_ui->power_->shutdown();
    }
#endif
}

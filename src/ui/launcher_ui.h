#pragma once

#include <rakos/app_registry.h>
#include <rakos/audio_service.h>
#include <rakos/boot_manager.h>
#include <rakos/display_manager.h>
#include <rakos/imu_service.h>
#include <rakos/input_manager.h>
#include <rakos/os_config.h>
#include <rakos/power_service.h>
#include <rakos/radio_service.h>
#include <rakos/storage_service.h>

class LauncherUI {
public:
    void begin(DisplayManager *display,
               InputManager *input,
               StorageService *storage,
               AppRegistry *registry,
               OsConfig *config,
               PowerService *power,
               ImuService *imu,
               AudioService *audio,
               RadioService *radio);

    void onStorageReady(bool sd_ready);
    void onSdStateChanged();
    void setImuSample(const ImuSample &sample);

    void update();

private:
    enum class Screen {
        kHome,
        kApps,
        kSettings,
        kStatus,
    };

    static constexpr Screen kNavScreens[] = {Screen::kHome, Screen::kApps, Screen::kSettings, Screen::kStatus};

    DisplayManager *display_ = nullptr;
    InputManager *input_ = nullptr;
    StorageService *storage_ = nullptr;
    AppRegistry *registry_ = nullptr;
    OsConfig *config_ = nullptr;
    PowerService *power_ = nullptr;
    ImuService *imu_ = nullptr;
    AudioService *audio_ = nullptr;
    RadioService *radio_ = nullptr;

    lv_obj_t *wifi_switch_ = nullptr;
    lv_obj_t *ble_switch_ = nullptr;
    lv_obj_t *wifi_ssid_ta_ = nullptr;
    lv_obj_t *wifi_pass_ta_ = nullptr;
    lv_obj_t *ble_name_ta_ = nullptr;
    lv_obj_t *radio_status_label_ = nullptr;
    lv_obj_t *radio_keyboard_ = nullptr;

    Screen screen_ = Screen::kHome;
    lv_obj_t *root_ = nullptr;
    lv_obj_t *status_bar_ = nullptr;
    lv_obj_t *content_area_ = nullptr;
    lv_obj_t *nav_bar_ = nullptr;
    lv_obj_t *nav_btns_[4] = {};
    lv_obj_t *status_label_ = nullptr;
    lv_obj_t *clock_label_ = nullptr;
    lv_obj_t *home_time_label_ = nullptr;
    lv_obj_t *home_wifi_label_ = nullptr;
    lv_obj_t *battery_label_ = nullptr;
    lv_obj_t *brightness_slider_ = nullptr;
    lv_obj_t *volume_slider_ = nullptr;

    lv_obj_t *install_overlay_ = nullptr;
    lv_obj_t *install_title_ = nullptr;
    lv_obj_t *install_phase_ = nullptr;
    lv_obj_t *install_bar_ = nullptr;
    lv_obj_t *install_pct_ = nullptr;

    bool screen_blank_ = false;
    uint8_t saved_brightness_pct_ = 80;
    uint32_t last_status_ms_ = 0;
    uint32_t last_radio_ui_ms_ = 0;

    void buildShell();
    void showScreen(Screen screen);
    void setNavActive(Screen screen);
    void refreshStatusBar();
    void refreshHomeWidgets();
    void refreshStatus();
    void refreshRadioStatusLabel();
    void buildHome();
    void buildApps();
    void buildSettings();
    void buildStatus();
    void refreshApps();
#ifdef RAKOS_UI_APP_GRID
    void buildAppGridPage();
    static void on_app_grid_refresh(lv_event_t *e);
#endif
    void pumpUi();
    void showInstallOverlay(const AppEntry &app);
    void updateInstallOverlay(const char *phase, size_t done, size_t total);
    void hideInstallOverlay();
    void showMessage(const char *title, const char *text);
    void launchSelectedApp(const AppEntry &app);
    void toggleScreenBlank();
    void showShutdownDialog();
    void handlePhysicalKeys();
    void applyBrightnessPct(uint8_t pct);
    void playFeedback(uint16_t freq_hz = 1040);

    static void on_nav_clicked(lv_event_t *e);
    static void on_home_apps(lv_event_t *e);
    static void on_home_run_app(lv_event_t *e);
    static void on_apps_refresh(lv_event_t *e);
    static void on_app_selected(lv_event_t *e);
    static void on_settings_mode(lv_event_t *e);
    static void on_brightness_changed(lv_event_t *e);
    static void on_volume_changed(lv_event_t *e);
    static void on_radio_save(lv_event_t *e);
    static void on_radio_ta_focus(lv_event_t *e);
    static void on_shutdown_btn(lv_event_t *e);
    static void on_shutdown_confirm(lv_event_t *e);
};

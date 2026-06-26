#pragma once

#include <rakos/app_registry.h>
#include <rakos/audio_service.h>
#include <rakos/display_manager.h>
#include <rakos/imu_service.h>
#include <rakos/input_manager.h>
#include <rakos/os_config.h>
#include <rakos/power_service.h>
#include <rakos/storage_service.h>
#include <rakos/radio_service.h>
#include "ui/launcher_ui.h"

struct RakosAppContext {
    DisplayManager *display = nullptr;
    InputManager *input = nullptr;
    StorageService *storage = nullptr;
    AppRegistry *registry = nullptr;
    OsConfig *config = nullptr;
    PowerService *power = nullptr;
    ImuService *imu = nullptr;
    AudioService *audio = nullptr;
    RadioService *radio = nullptr;
    LauncherUI *launcher = nullptr;
};

/** Start FreeRTOS worker tasks. On LCD-5 the launcher UI is built on the first LVGL task tick. */
void rakosStartTasks(RakosAppContext *ctx);

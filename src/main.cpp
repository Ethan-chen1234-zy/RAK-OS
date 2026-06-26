/**
 * RAKOS - ESP32-S3 launcher firmware entry (FreeRTOS tasks)
 */
#include <Arduino.h>
#include <rakos/boot_manager.h>
#include <rakos/display_manager.h>
#include <rakos/input_manager.h>
#include <rakos/io_expander.h>
#include <rakos/i2c_bus.h>
#include <rakos/os_config.h>
#include <rakos/storage_service.h>
#include <rakos/power_service.h>
#include <rakos/imu_service.h>
#include <rakos/audio_service.h>
#include <rakos/radio_service.h>
#include <rakos/app_registry.h>
#include "ui/launcher_ui.h"
#include "rtos_app.h"

#ifndef RAKOS_BUILD_TAG
#define RAKOS_BUILD_TAG __DATE__ " " __TIME__
#endif

static DisplayManager display;
static InputManager input;
static StorageService storage;
static PowerService power;
static ImuService imu;
static AudioService audio;
static RadioService radio;
static AppRegistry registry;
static OsConfig os_config;
static LauncherUI launcher;

static RakosAppContext app_ctx;

static void waitForUsbConsole() {
    Serial.begin(115200);
    const uint32_t start = millis();
    while (!Serial && (millis() - start) < 3000) {
        delay(10);
    }
    delay(200);
}

void setup() {
    waitForUsbConsole();

    Serial.println();
    Serial.println("=== RAKOS firmware (FreeRTOS) ===");
    Serial.printf("[BUILD] %s\n", RAKOS_BUILD_TAG);
    Serial.println("[BSP] PMIC polling disabled (shutdown only)");

#if !defined(RAKOS_BOARD_WAVESHARE_LCD5) && !defined(RAKOS_BOARD_WAVESHARE_LCD5B)
    rakos::i2cBusBegin();
#endif

    auto &expander = rakos::IoExpander::instance();
#if defined(RAKOS_BOARD_WAVESHARE_LCD5) || defined(RAKOS_BOARD_WAVESHARE_LCD5B)
    Serial.println("[BSP] LCD-5: ESP Panel owns I2C (no Wire before display)");
#else
    if (!expander.begin()) {
        Serial.println("[FATAL] IO expander init failed");
    } else {
        expander.boardPowerOn();
        Serial.println("[BSP] AMOLED power sequence done");
    }
#endif

    os_config.begin();
    BootManager::begin();

    if (!BootManager::isRunningOsSlot()) {
        Serial.println("[WARN] Not running from ota_0. Flash factory image to 0x0.");
    }

#if !defined(RAKOS_BOARD_WAVESHARE_LCD5) && !defined(RAKOS_BOARD_WAVESHARE_LCD5B)
    rakos::i2cBusRecover();
    input.init();

    if (!display.initTouch()) {
        Serial.println("[BSP] Touch init failed (will retry after SD mount)");
    }
#else
    input.init();
#endif

    if (!display.init()) {
        Serial.println("[FATAL] Display init failed");
#if defined(RAKOS_BOARD_WAVESHARE_LCD5) || defined(RAKOS_BOARD_WAVESHARE_LCD5B)
        return;
#endif
    }

    display.startLvglTick();

    registry.begin(false);

    app_ctx.display = &display;
    app_ctx.input = &input;
    app_ctx.storage = &storage;
    app_ctx.registry = &registry;
    app_ctx.config = &os_config;
    app_ctx.power = &power;
    app_ctx.imu = &imu;
    app_ctx.audio = &audio;
    app_ctx.radio = &radio;
    app_ctx.launcher = &launcher;

#if !defined(RAKOS_BOARD_WAVESHARE_LCD5) && !defined(RAKOS_BOARD_WAVESHARE_LCD5B)
    launcher.begin(&display, &input, &storage, &registry, &os_config, &power, &imu, &audio, &radio);

#if defined(RAKOS_UI_APP_GRID)
    constexpr int kBootPumpFrames = 80;
#else
    constexpr int kBootPumpFrames = 40;
#endif
    for (int i = 0; i < kBootPumpFrames; ++i) {
        display.runLvgl();
        delay(5);
    }
    {
        const uint8_t pct = display.getBrightnessPercentage();
        const uint8_t level = (uint8_t)(((uint16_t)pct * 255 + 50) / 100);
        display.setBrightness(level);
    }
    display.forceRefresh();
#endif

    rakosStartTasks(&app_ctx);

    Serial.println("[OK] FreeRTOS scheduler running");
}

void loop() {
    vTaskDelay(pdMS_TO_TICKS(1000));
}

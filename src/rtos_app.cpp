#include "rtos_app.h"
#include <rakos/i2c_bus.h>
#include <rakos/imu_service.h>
#include <Arduino.h>
#include <lvgl.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#if defined(RAKOS_BOARD_WAVESHARE_LCD5) || defined(RAKOS_BOARD_WAVESHARE_LCD5B)
#include "lvgl_v8_port.h"
#endif

static RakosAppContext *g_ctx = nullptr;
static EventGroupHandle_t g_events = nullptr;

static constexpr uint32_t kEvtStorageReady = (1u << 0);
static constexpr uint32_t kEvtSdChanged = (1u << 1);

static constexpr UBaseType_t kPrioLvgl = 5;
static constexpr UBaseType_t kPrioStorage = 3;
static constexpr UBaseType_t kPrioSensor = 2;

#if defined(RAKOS_BOARD_WAVESHARE_LCD5) || defined(RAKOS_BOARD_WAVESHARE_LCD5B)
static bool g_launcher_ui_built = false;

static void buildLauncherUiOnce() {
    if (g_launcher_ui_built || !g_ctx || !g_ctx->launcher || !lvgl_port_is_ready()) {
        return;
    }
    if (!lvgl_port_lock(-1)) {
        return;
    }

    g_ctx->launcher->begin(g_ctx->display,
                           g_ctx->input,
                           g_ctx->storage,
                           g_ctx->registry,
                           g_ctx->config,
                           g_ctx->power,
                           g_ctx->imu,
                           g_ctx->audio,
                           g_ctx->radio);
    g_launcher_ui_built = true;
    lvgl_port_unlock();
    Serial.println("[BSP] Launcher UI ready");
}
#endif

static void lvglTask(void *param) {
    (void)param;
    Serial.println("[RTOS] LVGL task start");

    for (;;) {
#if defined(RAKOS_BOARD_WAVESHARE_LCD5) || defined(RAKOS_BOARD_WAVESHARE_LCD5B)
        if (lvgl_port_is_ready()) {
            buildLauncherUiOnce();
        }
#endif
        if (g_ctx && g_ctx->input) {
            g_ctx->input->update();
        }

        EventBits_t bits = xEventGroupGetBits(g_events);

#if defined(RAKOS_BOARD_WAVESHARE_LCD5) || defined(RAKOS_BOARD_WAVESHARE_LCD5B)
        // LVGL timer + touch polling run in lvgl_port_task; only touch UI here under lock.
        if (lvgl_port_is_ready() && lvgl_port_lock(100)) {
            if ((bits & kEvtStorageReady) && g_ctx && g_ctx->launcher) {
                xEventGroupClearBits(g_events, kEvtStorageReady);
                g_ctx->launcher->onStorageReady(g_ctx->storage && g_ctx->storage->sdReady());
                Serial.println("[RTOS] Storage ready handled in LVGL task");
            }
            if ((bits & kEvtSdChanged) && g_ctx && g_ctx->launcher) {
                xEventGroupClearBits(g_events, kEvtSdChanged);
                g_ctx->launcher->onSdStateChanged();
            }
            if (g_ctx && g_ctx->launcher) {
                g_ctx->launcher->update();
            }
            lvgl_port_unlock();
        }
        vTaskDelay(pdMS_TO_TICKS(5));
#else
        if ((bits & kEvtStorageReady) && g_ctx && g_ctx->launcher) {
            xEventGroupClearBits(g_events, kEvtStorageReady);
            g_ctx->launcher->onStorageReady(g_ctx->storage && g_ctx->storage->sdReady());
            Serial.println("[RTOS] Storage ready handled in LVGL task");
        }
        if ((bits & kEvtSdChanged) && g_ctx && g_ctx->launcher) {
            xEventGroupClearBits(g_events, kEvtSdChanged);
            g_ctx->launcher->onSdStateChanged();
        }

        if (g_ctx && g_ctx->launcher) {
            g_ctx->launcher->update();
        }

        if (g_ctx && g_ctx->display) {
            const uint32_t delay_ms = lv_timer_handler();
#if defined(RAKOS_UI_APP_GRID)
            const uint32_t wait_ms = delay_ms > 0 ? (delay_ms < 8 ? delay_ms : 8) : 4;
#else
            const uint32_t wait_ms = delay_ms > 0 ? (delay_ms < 5 ? delay_ms : 5) : 3;
#endif
            vTaskDelay(pdMS_TO_TICKS(wait_ms));
        } else {
            vTaskDelay(pdMS_TO_TICKS(5));
        }
#endif
    }
}

static void storageTask(void *param) {
    (void)param;
    Serial.println("[RTOS] Storage task start");
    vTaskDelay(pdMS_TO_TICKS(300));

    if (!g_ctx || !g_ctx->storage) {
        vTaskDelete(nullptr);
        return;
    }

    g_ctx->storage->init();
    // Touch and PMIC are already initialized during boot.
    // Rebuilding shared I2C clients after SD mount caused invalid-state
    // failures inside third-party drivers, so keep the live bus state.

    if (g_ctx->registry && g_ctx->storage) {
        g_ctx->registry->begin(g_ctx->storage->sdReady());
    }

    if (g_ctx->power && !g_ctx->power->begin()) {
        Serial.println("[BSP] AXP2101 init after SD failed");
    }

    if (g_ctx->imu) {
        g_ctx->imu->begin();
    }

    if (g_ctx->audio && !g_ctx->audio->ready()) {
        if (g_ctx->audio->begin()) {
            g_ctx->audio->beep(1040, 48);
        }
    }

    if (g_ctx->radio) {
        g_ctx->radio->begin();
        if (g_ctx->storage && g_ctx->storage->sdReady()) {
            if (g_ctx->radio->tryApplyWifiFromSd()) {
                Serial.println("[RTOS] WiFi config loaded from SD");
            }
        }
    }

    xEventGroupSetBits(g_events, kEvtStorageReady);
    Serial.println("[RTOS] Storage task done");
    vTaskDelete(nullptr);
}

static void sensorTask(void *param) {
    (void)param;
    Serial.println("[RTOS] Sensor task start");
    vTaskDelay(pdMS_TO_TICKS(3000));

    uint32_t last_sd_poll_ms = 0;

    for (;;) {
        const uint32_t now = millis();

        if (g_ctx && g_ctx->storage && (now - last_sd_poll_ms) >= 15000) {
            last_sd_poll_ms = now;
            if (g_ctx->storage->updateSdState()) {
                if (g_ctx->registry) {
                    g_ctx->registry->begin(g_ctx->storage->sdReady());
                }
                if (g_events) {
                    xEventGroupSetBits(g_events, kEvtSdChanged);
                }
            }
        }

        if (g_ctx && g_ctx->imu && g_ctx->launcher) {
            ImuSample sample;
            if (g_ctx->imu->update(sample)) {
                g_ctx->launcher->setImuSample(sample);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

#if defined(RAKOS_UI_APP_GRID)
static constexpr uint32_t kLvglStackWords = 24576;
#else
static constexpr uint32_t kLvglStackWords = 16384;
#endif

void rakosStartTasks(RakosAppContext *ctx) {
    g_ctx = ctx;
    g_events = xEventGroupCreate();

    xTaskCreatePinnedToCore(lvglTask, "lvgl", kLvglStackWords, nullptr, kPrioLvgl, nullptr, 1);
    xTaskCreatePinnedToCore(storageTask, "storage", 12288, nullptr, kPrioStorage, nullptr, 0);
    xTaskCreatePinnedToCore(sensorTask, "sensor", 4096, nullptr, kPrioSensor, nullptr, 0);

    Serial.println("[RTOS] Tasks created (lvgl@1, storage/sensor@0)");
}

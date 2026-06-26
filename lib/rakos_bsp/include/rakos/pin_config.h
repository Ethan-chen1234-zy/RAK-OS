#pragma once

#if defined(RAKOS_BOARD_WAVESHARE_LCD5) || defined(RAKOS_BOARD_WAVESHARE_LCD5B)
#include "pin_config_lcd5.h"
#if defined(RAKOS_BOARD_WAVESHARE_LCD5B)
#undef LCD_WIDTH
#undef LCD_HEIGHT
#define LCD_WIDTH  1024
#define LCD_HEIGHT 600
#endif
#else
/**
 * Waveshare ESP32-S3-Touch-AMOLED-1.8
 * Panel: 368x448 AMOLED (SH8601, QSPI)
 * Touch: CST816 @ I2C 0x15 (demo: Arduino_DriveBus)
 * IO expander: TCA9554 @ 0x20
 */

#define RAKOS_HAS_BOOT_BUTTON     1
#define RAKOS_HAS_PWR_BUTTON      1
#define RAKOS_HAS_AXP_PMU         1
#define RAKOS_HAS_IMU             1
#define RAKOS_HAS_AUDIO           1

/* ---------- Buttons ---------- */
#define BUTTON_BOOT           0   /* GPIO0, active low */

/* ---------- AMOLED 1.8" 368x448 QSPI ---------- */
#define LCD_WIDTH             368
#define LCD_HEIGHT            448
/** Rounded AMOLED corners — keep UI content inside this inset */
#define UI_SAFE_INSET         12
#define LCD_SPI_HOST          SPI2_HOST
#define LCD_PIXEL_CLK_HZ      40000000
#define LCD_DRAW_BUFF_HEIGHT  80

#define LCD_CS                12
#define LCD_SCLK              11
#define LCD_SDIO0             4
#define LCD_SDIO1             5
#define LCD_SDIO2             6
#define LCD_SDIO3             7
#define LCD_RST               (-1)

/* ---------- I2C (touch, PMIC, RTC, IMU, IO expander) ---------- */
#define I2C_SDA               15
#define I2C_SCL               14
#define TOUCH_I2C_ADDR        0x15
#define TOUCH_INT             21
#define TOUCH_RST             (-1)

/* ---------- TCA9554 IO expander (8-bit) ---------- */
#define IOEXP_I2C_ADDR        0x20
#define EXPIO_LCD_RESET       0
#define EXPIO_DSI_PWR_EN      1
#define EXPIO_TP_RESET        2
#define EXPIO_PWR_BUTTON      4
#define EXPIO_SD_CS           7

/* ---------- microSD (SDMMC 1-bit) ---------- */
#define SD_PIN_CLK            2
#define SD_PIN_CMD            1
#define SD_PIN_D0             3
#define SD_MOUNT_POINT        "/sdcard"
/** VFS path for open()/scan — AMOLED SDMMC uses "/" (not "/sdcard") on this BSP */
#define SD_FS_ROOT            "/"
#define SD_MAX_FILES          16

/* ---------- SPIFFS on flash "storage" partition ---------- */
#define SPIFFS_MOUNT_POINT    "/spiffs"

/* ---------- AXP2101 PMIC ---------- */
#define AXP2101_I2C_ADDR      0x34

/* ---------- QMI8658 IMU ---------- */
#define QMI8658_I2C_ADDR      0x6B

/* ---------- ES8311 audio codec (I2S) ---------- */
#define I2S_MCLK              16
#define I2S_BCLK              9
#define I2S_WS                45
#define I2S_DOUT              8
#define I2S_DIN               10
#define I2S_PA_PIN            46
#define ES8311_I2C_ADDR       0x18
#define AUDIO_DEFAULT_RATE    16000
#endif

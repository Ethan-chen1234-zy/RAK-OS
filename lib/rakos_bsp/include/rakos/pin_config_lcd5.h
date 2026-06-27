#pragma once

/**
 * Waveshare ESP32-S3-Touch-LCD-5 (800x480 RGB / ST7262 + GT911 + CH422G)
 * Docs: https://docs.waveshare.net/ESP32-S3-Touch-LCD-5/
 */

#define RAKOS_HAS_BOOT_BUTTON     0
#define RAKOS_HAS_PWR_BUTTON      0
#define RAKOS_HAS_AXP_PMU         0
#define RAKOS_HAS_IMU             0
#define RAKOS_HAS_AUDIO           0
#define RAKOS_HAS_IO_EXPANDER     1
#define RAKOS_HAS_RGB_LED         0
#define RAKOS_HAS_USB_UART        1
#define RAKOS_HAS_SD_CARD         1
#define RAKOS_HAS_BUZZER          1
#define RAKOS_HAS_RS485           1
#define RAKOS_HAS_CAN             1
#define RAKOS_HAS_RTC             1

#define LCD_WIDTH                 800
#define LCD_HEIGHT                480
#define UI_SAFE_INSET             8

/* I2C: GT911 + CH422G + PCF85063A */
#define I2C_SDA                   8
#define I2C_SCL                   9
#define TOUCH_INT                 4
#define TOUCH_RST                 (-1)

/* CH422G IO expander (via ESP32_IO_Expander) */
#define IOEXP_I2C_ADDR            0x20
#define EXPIO_TP_RESET            1
#define EXPIO_LCD_BL              2
#define EXPIO_LCD_RESET           3
#define EXPIO_SD_CS               4
#define EXPIO_USB_SEL             5
/* CH422G open-collector outputs (see Waveshare IO_Test DO0/DO1) */
#define EXPIO_DO0                 8
#define EXPIO_DO1                 9
#define EXPIO_BUZZER_DO           EXPIO_DO0

/* RS485 / CAN (terminal block) */
#define RS485_RX_PIN              43
#define RS485_TX_PIN              44
#define CAN_TX_PIN                15
#define CAN_RX_PIN                16

/* microSD (SPI, CS on CH422G) */
#define RAKOS_SD_USE_SPI          1
#define SD_SPI_HOST               SPI2_HOST
#define SD_PIN_MOSI               11
#define SD_PIN_MISO               13
#define SD_PIN_CLK                12
#define SD_PIN_CS                 (-1)
#define SD_MOUNT_POINT            "/sdcard"
#define SD_FS_ROOT                "/sdcard"
#define SD_MAX_FILES              16

#define SPIFFS_MOUNT_POINT        "/spiffs"

/* Placeholder audio rate when codec absent */
#define AUDIO_DEFAULT_RATE        16000

/* Placeholder — BOOT not wired on RGB pin map */
#define BUTTON_BOOT               (-1)

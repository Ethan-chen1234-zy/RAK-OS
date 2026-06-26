# RAKOS Architecture



ESP32-S3 launcher OS for **Waveshare ESP32-S3-Touch-AMOLED-1.8** (368×448 SH8601 QSPI + CST816 touch).



## Layer diagram



```text

+--------------------------------------------------+

|  src/main.cpp          Entry / wiring            |

|  src/ui/launcher_ui    LVGL screens              |

+--------------------------------------------------+

|  lib/rakos_apps        SD app registry           |

|  lib/rakos_core        Boot + NVS config         |

|  lib/rakos_bsp         HW: LCD/touch/IO/SD       |

|  lib/rakos_app_policy  (user apps only) rollback |

+--------------------------------------------------+

|  Flash layout (partitions_rakos.csv)             |

|    ota_0 @ 0x20000   RAKOS firmware              |

|    ota_1 @ 0x400000  User application             |

|    storage           SPIFFS config/cache         |

|    microSD           App packages + assets       |

+--------------------------------------------------+

```



## Flash roles



| Region | Offset | Role |

|--------|--------|------|

| `ota_0` | `0x20000` | RAKOS launcher (this project) |

| `ota_1` | `0x400000` | User app binary |

| `storage` | `0xC00000` | SPIFFS (`/spiffs`) |

| microSD | — | `/sdcard/<Category>/<App>/app.bin` |



## Boot flow



1. Bootloader reads `otadata`, starts `ota_0` or `ota_1`.

2. RAKOS runs in `ota_0`, shows LVGL launcher.

3. User selects app → copy `app.bin` from SD to `ota_1` → `esp_ota_set_boot_partition(ota_1)` → reboot.

4. User app runs in `ota_1`. With `rakos_app_policy`, image stays unverified → reset returns to OS.

5. Exit: **BOOT (GPIO0)** or **PWR (EXIO4)** → OS sets next boot to `ota_0`.



## SD app layout



```text

/sdcard/

  General/

    Camera/

      app.bin

  Games/

    Pong/

      app.bin

```



## Build targets



| Environment | Upload offset | Image |

|-------------|---------------|-------|

| `rakos_os_local` | `0x20000` | RAKOS OS |

| `rakos_os` | `0x20000` | RAKOS OS (online platform) |



User apps: separate project, upload to `0x400000`, link `rakos_app_policy` + wrap flag.



## Hardware (rakos_bsp)



Target board: [Waveshare ESP32-S3-Touch-AMOLED-1.8](https://www.waveshare.com/wiki/ESP32-S3-Touch-AMOLED-1.8)



| Function | Pins / device |

|----------|----------------|

| LCD QSPI | CS=12, SCLK=11, D0–D3=4/5/6/7 |

| Panel | 368×448 SH8601 (`Arduino_CO5300` driver) |

| I2C | SDA=15, SCL=14 |

| Touch | CST816 @ 0x15, INT=GPIO21 (Arduino_DriveBus) |

| IO expander | TCA9554 @ 0x20 (LCD/DSI/TP power, PWR btn, SD CS) |

| BOOT button | GPIO0 (active low) |

| PWR button | EXIO4 (active high) |

| microSD | SDMMC 1-bit CLK=2, CMD=1, D0=3 |

| Flash / PSRAM | 16 MB / 8 MB OPI |

| AXP2101 PMIC | I2C `0x34` — battery %, charge, VBUS |

| QMI8658 IMU | I2C `0x6B` — accel + gyro |

| ES8311 audio | I2S MCLK=16, BCLK=9, WS=45, DOUT=8, DIN=10, PA=46 |

## BSP services (`lib/rakos_bsp`)

| Service | API | Notes |
|---------|-----|-------|
| `PowerService` | `begin()`, `update()`, `status()` | XPowersLib AXP2101 |
| `ImuService` | `begin()`, `update(ImuSample&)` | SensorLib QMI8658 |
| `AudioService` | `begin()`, `write()`, `read()`, `setVolume()` | ES8311 + ESP_I2S |

# RAKOS (ESP32-S3)

kodeOS-like launcher for **Waveshare ESP32-S3-Touch-AMOLED-1.8** (368×448 QSPI AMOLED + CST816 touch).

**Version**: see [VERSION](VERSION) (current `0.2.0-dev`)  
**Changelog**: [CHANGELOG.md](CHANGELOG.md)  
**中文说明**: [README.zh-CN.md](README.zh-CN.md)  
**Design docs**: [docs/overview.md](docs/overview.md) (start here) · [docs/](docs/)  
**Architecture**: [ARCHITECTURE.md](ARCHITECTURE.md)

## Features

- **BSP**: QSPI AMOLED, CST816 touch, TCA9554, AXP2101 PMIC, QMI8658 IMU, ES8311 audio, SD + SPIFFS
- **Core**: dual OTA boot manager, NVS config (`maker` / `managed`)
- **Apps**: scan SD categories, flash `app.bin` → `ota_1`, launch
- **UI**: LVGL launcher (Home / Apps / Settings / Status); Setup configures WiFi + BLE; status bar clock after NTP sync
- **App UI** (`rakos_os_appui` / `rakos_os_lcd5_appui`): Home 保持原样；**Apps** 页为图标网格，点击启动应用

## Quick start

```bash
pio run -e rakos_os_local
pio run -e rakos_os_local -t upload
# or icon-grid launcher:
pio run -e rakos_os_appui -t upload
pio device monitor
```

OS firmware is written to **`0x20000` (`ota_0`)**. Module flash is **16 MB**.

## SD app package

On the **PC**, copy folders to the **TF card root** (FAT32):

```text
Games/Hello/app.bin
Games/Clock/app.bin
...
```

On the AMOLED board the VFS path is `/Games/Hello/app.bin` (root `/`, not `/sdcard/...`). In **Apps**, tap **SD Scan**, then tap the app tile (e.g. **Hello**).

Factory image (bootloader + partition table + OS): `RAKOS-AppUI.factory.bin` @ flash **0x0**.

> **Note:** A full OS reflash (`erase-all`) clears `ota_1`. You must install apps from SD again (or `pio run -e hello_app -t upload` for USB dev flash).

With **`rakos_os_appui`**, the Apps page uses an icon grid: SD apps paginate vertically; **Flash** / **SD Scan** are system actions at the top.

## WiFi and clock (SD)

Copy `sd/wifi.ini.example` to the **TF card root** as `wifi.ini` (same level as `Games/`):

```ini
ssid=RAK-VPN
password=RakVpn%20240828
```

**Yes — without `wifi.ini` (or Setup WiFi enabled), the device will not connect.** Default is WiFi off in NVS.

On boot RAKOS scans nearby APs, connects to the configured SSID, then syncs time (NTP, UTC+8). The status bar shows **HH:MM** next to `RAKOS` when synced.

You can also set WiFi in **Setup → Save WiFi / BLE** (saved to NVS and written to `wifi.ini` when SD is mounted).

## Adapted applications

User apps run in **`ota_1` (`0x400000`)**. Build `app.bin`, copy to SD, then in RAKOS **Apps** tap the app tile (or use **SD Scan** after copying).

### AMOLED 1.8" (368×448) — in this repo

| App | Build env | SD path | Description |
|-----|-----------|---------|-------------|
| **Hello** | `hello_app` | `Games/Hello/app.bin` | Minimal LVGL demo, uptime counter |
| **Clock** | `clock_app` | `Games/Clock/app.bin` | Analog + digital clock (RTC when available) |
| **Btn** | `btn_app` | `Games/Btn/app.bin` | Colored buttons LVGL demo |
| **Meter** | `meter_app` | `Games/Meter/app.bin` | Arc gauge animation |
| **Slider** | `slider_app` | `Games/Slider/app.bin` | Brightness / volume sliders |

Build all AMOLED demos:

```bash
pio run -e hello_app -e clock_app -e btn_app -e meter_app -e slider_app
```

Artifacts: `examples/<AppName>/dist/app.bin` (e.g. `examples/BtnApp/dist/app.bin`).

Exit back to RAKOS: **BOOT** hold ~1.5 s (AMOLED). Details per app: [examples/HelloApp/README.md](examples/HelloApp/README.md), [examples/ClockApp/README.md](examples/ClockApp/README.md).

### AMOLED — third-party (external repo)

| App | Build | SD path | Notes |
|-----|-------|---------|-------|
| **Meshtastic** | In [Meshtastic firmware-add-variants](https://github.com/meshtastic/firmware): `pio run -e waveshare-amoled-18-rakos` | `Games/Meshtastic/app.bin` | LoRa mesh radio firmware packaged for RAKOS `ota_1`. **No onboard SX1262** on the 1.8" board — LoRa disabled by default; external module optional. BOOT hold ~3 s returns to RAKOS. Experimental. |

Output: `variants/esp32s3/waveshare_esp32s3_touch_amoled_1_8/dist/app.bin`.

### LCD-5 (800×480) — in this repo

| App | Build env | SD path | Description |
|-----|-----------|---------|-------------|
| **Hello** | `hello_app_lcd5` | `Games/Hello/app.bin` | Same Hello demo, LVGL 8 + GT911 |
| **Clock** | `clock_app_lcd5` | `Games/Clock/app.bin` | Same Clock demo for LCD-5 |

```bash
pio run -e hello_app_lcd5 -e clock_app_lcd5
```

Artifacts: `examples/HelloApp/dist_lcd5/app.bin`, `examples/ClockApp/dist_lcd5/app.bin`.

Btn / Meter / Slider demos are **AMOLED-only** for now (no `*_lcd5` env yet). Exit via on-screen **RAKOS** button.

## Waveshare ESP32-S3-Touch-LCD-5 (800×480)

OS firmware:

```bash
pio run -e rakos_os_lcd5 -t upload
# or icon-grid launcher (Apps page):
pio run -e rakos_os_lcd5_appui -t upload
```

Flash `RAKOS-LCD5.factory.bin` at **0x0** (factory image includes bootloader + partition table).

User apps: see **LCD-5** table under [Adapted applications](#adapted-applications). LCD-5 apps use **LVGL 8** + GT911 touch.

## User applications

Separate PlatformIO/Arduino project:

- Upload to `0x400000`
- Add `lib/rakos_app_policy` (or copy `ota_policy_override.cpp`)
- Build flag: `-Wl,--wrap=esp_ota_mark_app_valid_cancel_rollback`
- Use the same Waveshare pin map in your app BSP

## Project layout

```text
lib/rakos_bsp/       Display, touch, input, storage
lib/rakos_core/      BootManager, OsConfig
lib/rakos_apps/      AppRegistry (SD scan)
lib/rakos_app_policy User-app rollback hooks
src/ui/              LVGL launcher
examples/HelloApp/   Demo user app (SD / ota_1 test)
examples/ClockApp/   Watch demo (analog + digital clock)
examples/BtnApp/     Button widgets demo
examples/MeterApp/   Arc gauge demo
examples/SliderApp/  Slider widgets demo
partitions_rakos.csv Flash partition table
boards/rakos_esp32s3.json
```

## Environments

| Env | Platform | Upload |
|-----|----------|--------|
| `rakos_os_local` | Local Arduino15 BSP | `0x20000` |
| `rakos_os_appui` | Same as local, icon-grid UI | `0x20000` |
| `rakos_os` | pioarduino online | `0x20000` |
| `rakos_os_lcd5` | Waveshare LCD-5 800×480 | `0x0` factory |
| `rakos_os_lcd5_appui` | LCD-5, icon-grid Apps page | `0x0` factory |
| `hello_app_lcd5` / `clock_app_lcd5` | LCD-5 user apps | `0x400000` |
| `hello_app` / `clock_app` / `btn_app` / `meter_app` / `slider_app` | AMOLED user apps | `0x400000` |

## Firmware backup

Pre-built backups with version manifest: `dist/firmware_backup/`. To capture a local build:

```bash
pio run -e rakos_os_appui
# .pio/build/rakos_os_appui/RAKOS-AppUI.bin
# .pio/build/rakos_os_appui/RAKOS-AppUI.factory.bin
```

## Documentation

| Doc | Topic |
|-----|-------|
| [docs/design.md](docs/design.md) | RAKOS design background & principles |
| [docs/kodeos-reference.md](docs/kodeos-reference.md) | kodeOS / Kode Dot public docs digest |
| [docs/comparison.md](docs/comparison.md) | RAKOS vs kodeOS & alternatives |
| [docs/rakos-kodeos-gap-analysis.zh-CN.md](docs/rakos-kodeos-gap-analysis.zh-CN.md) | Current design gaps, shortcomings, and optimization advice (Chinese) |
| [CHANGELOG.md](CHANGELOG.md) | Release history |

中文文档：[docs/README.zh-CN.md](docs/README.zh-CN.md)

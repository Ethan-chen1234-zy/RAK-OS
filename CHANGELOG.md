# Changelog

All notable changes to RAKOS are documented in this file.

Format based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [0.2.0-dev] - 2026-06-26

### Added

- **App-grid launcher** (`rakos_os_appui`, `rakos_os_lcd5_appui`): icon tiles, vertical paging, system row for **Flash** / **SD Scan**
- **Demo apps**: BtnApp, MeterApp, SliderApp (`btn_app`, `meter_app`, `slider_app` PlatformIO envs)
- **WiFi from SD**: `wifi.ini` on TF card root (`ssid` / `password`); boot scan + connect + NTP (UTC+8)
- **Clock on UI**: status bar `HH:MM`; Home page date/time and WiFi status
- **LCD-5 support**: `rakos_os_lcd5`, app-grid variant, `hello_app_lcd5` / `clock_app_lcd5`
- **Documentation**: `docs/` design notes, kodeOS reference, comparison; `README.zh-CN.md`
- **Firmware backup**: `dist/firmware_backup/` with version manifest

### Changed

- SD VFS root for AMOLED: file paths use `/` (e.g. `/Games/Hello/app.bin`, `/wifi.ini`)
- App registry and `flashAppBinToOta1` use robust SD path resolution
- SD presence probe: fixed false unmount loop when `/sdcard` directory open failed
- README: adapted apps table, WiFi setup, LCD-5 section

### Fixed

- HelloApp and other SD apps not found / flash failed (wrong `/sdcard` scan root)
- `btn_app` / `meter_app` / `slider_app` build: added per-app `lv_conf.h` and `-include`
- PlatformIO penv `uv` timeout on Windows (documented in `dist/BUILD_NOTES.txt`)

### Known issues

- Launching an app **copies full `app.bin` to `ota_1`** each time (slow; SD must stay mounted)
- `OsMode::Maker` exists in NVS but USB “Create App” workflow not implemented yet
- No per-app `manifest.json` / custom icons (folder name + letter tile only)
- Meshtastic as RAKOS app: experimental; no onboard SX1262 on AMOLED 1.8"
- LCD-5: Btn/Meter/Slider demos not ported (`*_lcd5` envs missing)

## [0.1.0] - 2026-06 (initial)

### Added

- Dual OTA layout: `ota_0` RAKOS @ `0x20000`, `ota_1` user app @ `0x400000`
- LVGL launcher: Home / Apps / Setup / Info
- SD app registry (General, Games, GPIO, USB, Hacking categories)
- BSP: AMOLED 368×448, touch, PMIC, IMU, audio, SDMMC, SPIFFS
- HelloApp and ClockApp examples
- WiFi + BLE setup (NVS), `RadioService`
- `rakos_app_policy` for user-app return to OS without OTA rollback verify

[0.2.0-dev]: https://github.com/
[0.1.0]: https://github.com/

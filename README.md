# RAKOS (ESP32-S3)

RAKOS is a lightweight ESP32-S3 touch launcher firmware with an LVGL home UI, dual OTA boot flow, user app installation, WiFi/BLE setup, and board-level BSP support.

Currently targeted boards:

- Waveshare ESP32-S3-Touch-AMOLED-1.8
- Waveshare ESP32-S3-Touch-LCD-5 / LCD-5B
- ESP32-S3-DevKitC-1 N16R8 + PY206-W38-V2 AMOLED adapter

**Version**: see [VERSION](VERSION)  
**Changelog**: [CHANGELOG.md](CHANGELOG.md)  
**Documentation**: [docs/README.md](docs/README.md)  
**中文说明**: [README.zh-CN.md](README.zh-CN.md)

## Quick Start

Use the project-local `pio` wrapper to avoid Python/PlatformIO version drift:

```powershell
.\pio run
.\pio run -t upload --upload-port COM5
.\pio device monitor -b 115200
```

Default environment: `rakos_os_appui`. Common environments:

```powershell
# Default Waveshare 1.8 AMOLED icon-grid UI
.\pio run -e rakos_os_appui -t upload

# PY206-W38-V2 + ESP32-S3-DevKitC-1 N16R8
.\pio run -e rakos_os_py206 -t upload

# Waveshare LCD-5
.\pio run -e rakos_os_lcd5_appui -t upload
```

The OS firmware runs from `ota_0`. Factory images include the bootloader, partition table, and OS image and can be flashed from `0x0`.

## First Run

1. Pick the matching PlatformIO env for your hardware.
2. Flash the firmware and open the serial monitor.
3. Verify display and touch initialization logs.
4. Configure WiFi/BLE from the RAKOS `Setup` page.
5. On SD-capable boards, place `Games/<App>/app.bin` on the TF card and install from the `Apps` page.

Boards without SD, such as the current direct-wired PY206 setup, can still store WiFi credentials in NVS from the Setup page.

## Useful Docs

- [Quick start](docs/quick-start.zh-CN.md)
- [Supported boards and environments](docs/boards.zh-CN.md)
- [PY206-W38-V2 hardware notes](docs/hardware-py206.zh-CN.md)
- [WiFi configuration](docs/wifi.zh-CN.md)
- [User apps and SD packages](docs/apps.zh-CN.md)
- [Porting a new board](docs/porting-board.zh-CN.md)
- [Design notes](docs/design.md)

## Layout

```text
src/                  RAKOS firmware and LVGL UI
lib/rakos_bsp/        Display, touch, input, storage, radio services
lib/rakos_core/       Boot manager and OS config
lib/rakos_apps/       App scan, install, and registry
lib/rakos_app_policy/ User-app rollback policy
examples/             Example user apps
docs/                 Usage, porting, design, and hardware docs
```

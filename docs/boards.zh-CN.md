# 已支持设备与构建环境

RAKOS 通过 PlatformIO env 和 `RAKOS_BOARD_*` 宏区分硬件。新增设备时应优先新增 env 和引脚配置分支，避免修改既有板子的默认逻辑。

## OS 固件环境

| 设备 | Env | 说明 |
|------|-----|------|
| Waveshare ESP32-S3-Touch-AMOLED-1.8 | `rakos_os_appui` | 默认环境，QSPI AMOLED，图标网格 Apps |
| Waveshare ESP32-S3-Touch-AMOLED-1.8 | `rakos_os_local` | 本地平台基础 OS |
| Waveshare ESP32-S3-Touch-AMOLED-1.8 | `rakos_os` | pioarduino 在线平台 |
| ESP32-S3-DevKitC-1 N16R8 + PY206-W38-V2 | `rakos_os_py206` | 飞线转接屏，CO5300 + CST92xx，无 SD |
| Waveshare ESP32-S3-Touch-LCD-5 | `rakos_os_lcd5_appui` | LCD-5 图标网格 UI |
| Waveshare ESP32-S3-Touch-LCD-5 | `rakos_os_lcd5` | LCD-5 基础 OS |
| Waveshare ESP32-S3-Touch-LCD-5B | `rakos_os_lcd5b` | 1024x600 LCD-5B |

## 用户应用环境

| 应用 | AMOLED 1.8 Env | LCD-5 Env |
|------|----------------|-----------|
| Hello | `hello_app` | `hello_app_lcd5` |
| Clock | `clock_app` | `clock_app_lcd5` |
| Btn | `btn_app` | 暂无 |
| Meter | `meter_app` | 暂无 |
| Slider | `slider_app` | 暂无 |

## 烧录地址

- OS 固件运行在 `ota_0`
- 用户应用运行在 `ota_1`，地址 `0x400000`
- factory 镜像从 `0x0` 烧录，包含 bootloader、分区表和 OS

## 能力宏

硬件能力应放在 `lib/rakos_bsp/include/rakos/pin_config.h` 或对应 board 配置中：

```cpp
#define RAKOS_HAS_BOOT_BUTTON 1
#define RAKOS_HAS_PWR_BUTTON  0
#define RAKOS_HAS_IO_EXPANDER 0
#define RAKOS_HAS_SD_CARD     0
#define RAKOS_HAS_AUDIO       0
#define RAKOS_HAS_IMU         0
```

业务逻辑应优先判断能力宏，而不是直接判断具体板名。

## 相关文档

- [PY206-W38-V2 硬件适配](hardware-py206.zh-CN.md)
- [快速适配新设备](porting-board.zh-CN.md)
- [快速使用](quick-start.zh-CN.md)

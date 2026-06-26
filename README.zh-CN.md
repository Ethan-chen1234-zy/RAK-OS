# RAKOS (ESP32-S3)

面向 **Waveshare ESP32-S3-Touch-AMOLED-1.8**（368×448 QSPI AMOLED + CST816 触摸）的类 kodeOS 启动器固件。

**版本**：见 [VERSION](VERSION)（当前 `0.2.0-dev`）  
**更新记录**：[CHANGELOG.md](CHANGELOG.md)  
**设计文档**：[docs/overview.zh-CN.md](docs/overview.zh-CN.md)（项目简介）· [docs/README.zh-CN.md](docs/README.zh-CN.md)（完整索引）  
**架构说明**：[ARCHITECTURE.md](ARCHITECTURE.md)

## 功能概览

- **BSP**：QSPI AMOLED、CST816 触摸、TCA9554、AXP2101 电源、QMI8658 IMU、ES8311 音频、SD + SPIFFS
- **核心**：双 OTA 启动管理、NVS 配置（`maker` / `managed`）
- **应用**：扫描 SD 卡分类目录，将 `app.bin` 刷入 `ota_1` 后启动
- **界面**：LVGL 启动器（主页 / 应用 / 设置 / 信息）；设置页可配置 WiFi + BLE；NTP 同步后状态栏显示时钟
- **应用网格 UI**（`rakos_os_appui` / `rakos_os_lcd5_appui`）：主页保持原样；**应用**页为图标网格，点击启动

## 快速开始

```bash
pio run -e rakos_os_local
pio run -e rakos_os_local -t upload
# 或使用图标网格启动器：
pio run -e rakos_os_appui -t upload
pio device monitor
```

OS 固件写入 **`0x20000`（`ota_0`）**。模组 Flash 为 **16 MB**。

出厂完整镜像（含 bootloader + 分区表）：`RAKOS-AppUI.factory.bin`，烧录地址 **0x0**。

## SD 卡应用包

在 **电脑上** 将文件夹复制到 **TF 卡根目录**（FAT32）：

```text
Games/Hello/app.bin
Games/Clock/app.bin
...
wifi.ini          # 可选，WiFi 配置
```

在设备上 AMOLED 板的路径为 **`/Games/Hello/app.bin`**（VFS 根为 `/`，不是 `/sdcard/...`）。在 **应用** 页点击 **SD Scan**，再点击应用图标（如 **Hello**）。

> **注意**：全片擦除（`erase-all`）会清空 `ota_1`，需重新从 SD 安装应用（或通过 `pio run -e hello_app -t upload` 用 USB 开发烧录）。

使用 **`rakos_os_appui`** 时，应用页为图标网格：SD 应用纵向分页；顶部 **Flash** / **SD Scan** 为系统操作。

## WiFi 与时钟（SD 配置）

将 `sd/wifi.ini.example` 复制到 **TF 卡根目录**，命名为 `wifi.ini`（与 `Games/` 同级）：

```ini
ssid=你的WiFi名称
password=你的密码
```

**没有 `wifi.ini`（且未在设置中启用 WiFi）时，设备不会自动联网。** NVS 默认 WiFi 关闭。

启动后 RAKOS 会扫描附近 AP、连接配置的 SSID，再通过 NTP 同步时间（UTC+8）。同步成功后状态栏在 `RAKOS` 旁显示 **HH:MM**；主页有大时钟与 WiFi 状态。

也可在 **设置 → Save WiFi / BLE** 中配置（写入 NVS，SD 挂载时同步写入 `wifi.ini`）。

## 已适配应用

用户应用运行在 **`ota_1`（`0x400000`）**。编译 `app.bin` 复制到 SD 后，在 RAKOS **应用** 页点击图标（或复制后先 **SD Scan**）。

### AMOLED 1.8"（368×448）— 本仓库

| 应用 | 编译环境 | SD 路径 | 说明 |
|------|----------|---------|------|
| **Hello** | `hello_app` | `Games/Hello/app.bin` | 最小 LVGL 演示，运行时间计数 |
| **Clock** | `clock_app` | `Games/Clock/app.bin` | 模拟 + 数字时钟 |
| **Btn** | `btn_app` | `Games/Btn/app.bin` | 彩色按钮 LVGL 演示 |
| **Meter** | `meter_app` | `Games/Meter/app.bin` | 圆弧仪表动画 |
| **Slider** | `slider_app` | `Games/Slider/app.bin` | 亮度/音量滑块演示 |

一键编译全部 AMOLED 演示：

```bash
pio run -e hello_app -e clock_app -e btn_app -e meter_app -e slider_app
```

产物：`examples/<AppName>/dist/app.bin`（如 `examples/BtnApp/dist/app.bin`）。

返回 RAKOS：长按 **BOOT** 约 1.5 秒（AMOLED）。各应用说明见 [examples/HelloApp/README.md](examples/HelloApp/README.md)、[examples/ClockApp/README.md](examples/ClockApp/README.md)。

### AMOLED — 第三方（外部仓库）

| 应用 | 编译 | SD 路径 | 备注 |
|------|------|---------|------|
| **Meshtastic** | [Meshtastic firmware-add-variants](https://github.com/meshtastic/firmware)：`pio run -e waveshare-amoled-18-rakos` | `Games/Meshtastic/app.bin` | 为 RAKOS `ota_1` 打包的 LoRa 网状固件。1.8" 板**无板载 SX1262**，默认关闭 LoRa；可外接模块。长按 BOOT ~3 秒回 OS。实验性。 |

### LCD-5（800×480）— 本仓库

| 应用 | 编译环境 | SD 路径 | 说明 |
|------|----------|---------|------|
| **Hello** | `hello_app_lcd5` | `Games/Hello/app.bin` | Hello 演示，LVGL 8 + GT911 |
| **Clock** | `clock_app_lcd5` | `Games/Clock/app.bin` | LCD-5 时钟演示 |

```bash
pio run -e hello_app_lcd5 -e clock_app_lcd5
```

产物：`examples/HelloApp/dist_lcd5/app.bin`、`examples/ClockApp/dist_lcd5/app.bin`。

Btn / Meter / Slider 演示**目前仅 AMOLED**（尚无 `*_lcd5` 环境）。LCD-5 通过屏幕 **RAKOS** 按钮退出。

## Waveshare ESP32-S3-Touch-LCD-5（800×480）

OS 固件：

```bash
pio run -e rakos_os_lcd5 -t upload
# 或图标网格启动器：
pio run -e rakos_os_lcd5_appui -t upload
```

烧录 `RAKOS-LCD5.factory.bin` 到 **0x0**（出厂镜像含 bootloader + 分区表）。

LCD-5 上 SD 路径为 `/sdcard/Games/...`。用户应用见上文 LCD-5 表格，使用 **LVGL 8** + GT911 触摸。

## 自行开发用户应用

独立 PlatformIO/Arduino 工程：

- 烧录地址 `0x400000`
- 加入 `lib/rakos_app_policy`（或复制 `ota_policy_override.cpp`）
- 编译选项：`-Wl,--wrap=esp_ota_mark_app_valid_cancel_rollback`
- 应用 BSP 使用与 Waveshare 一致的引脚定义

## 项目结构

```text
lib/rakos_bsp/       显示、触摸、输入、存储
lib/rakos_core/      BootManager、OsConfig
lib/rakos_apps/      AppRegistry（SD 扫描）
lib/rakos_app_policy 用户应用回滚策略
src/ui/              LVGL 启动器
examples/HelloApp/   演示用户应用
docs/                设计背景、kodeOS 参考、竞品分析
partitions_rakos.csv Flash 分区表
```

## 编译环境

| 环境 | 平台 | 烧录 |
|------|------|------|
| `rakos_os_local` | 本地 Arduino15 BSP | `0x20000` |
| `rakos_os_appui` | 同上，图标网格 UI | `0x20000` |
| `rakos_os` | pioarduino 在线 | `0x20000` |
| `rakos_os_lcd5` | Waveshare LCD-5 | `0x0` 出厂镜像 |
| `rakos_os_lcd5_appui` | LCD-5 图标网格 | `0x0` 出厂镜像 |
| `hello_app_lcd5` / `clock_app_lcd5` | LCD-5 用户应用 | `0x400000` |
| `hello_app` / `clock_app` / `btn_app` / `meter_app` / `slider_app` | AMOLED 用户应用 | `0x400000` |

## 固件备份

预编译备份（含版本清单）位于 `dist/firmware_backup/`。自行备份当前构建：

```bash
pio run -e rakos_os_appui
# 产物：.pio/build/rakos_os_appui/RAKOS-AppUI.bin
#       .pio/build/rakos_os_appui/RAKOS-AppUI.factory.bin
```

## 文档索引

| 文档 | 内容 |
|------|------|
| **[docs/overview.zh-CN.md](docs/overview.zh-CN.md)** | **项目简介**（演示 / 快速了解） |
| [docs/README.zh-CN.md](docs/README.zh-CN.md) | 文档目录（中文） |
| [docs/design.zh-CN.md](docs/design.zh-CN.md) | RAKOS 设计背景与原理 |
| [docs/kodeos-reference.zh-CN.md](docs/kodeos-reference.zh-CN.md) | kodeOS 公开资料摘要 |
| [docs/comparison.zh-CN.md](docs/comparison.zh-CN.md) | 与 kodeOS 等方案对比 |
| [docs/rakos-kodeos-gap-analysis.zh-CN.md](docs/rakos-kodeos-gap-analysis.zh-CN.md) | 当前设计差距、不足与优化建议 |
| [CHANGELOG.md](CHANGELOG.md) | 版本变更记录 |

英文文档：[docs/README.md](docs/README.md)

# RAKOS (ESP32-S3)

RAKOS 是一个面向 ESP32-S3 触摸屏设备的轻量启动器固件，提供 LVGL 桌面、双 OTA 启动、用户应用安装、WiFi/BLE 设置和基础 BSP 适配能力。

当前主要支持：

- Waveshare ESP32-S3-Touch-AMOLED-1.8
- Waveshare ESP32-S3-Touch-LCD-5 / LCD-5B
- ESP32-S3-DevKitC-1 N16R8 + PY206-W38-V2 AMOLED 转接屏

**版本**：见 [VERSION](VERSION)  
**更新记录**：[CHANGELOG.md](CHANGELOG.md)  
**完整文档**：[docs/README.zh-CN.md](docs/README.zh-CN.md)  
**English**：[README.md](README.md)

## 快速开始

请优先使用项目自带的 `pio` 包装脚本，避免系统 Python/PlatformIO 版本不一致：

```powershell
.\pio run
.\pio run -t upload --upload-port COM5
.\pio device monitor -b 115200
```

默认环境是 `rakos_os_appui`。常用环境：

```powershell
# 默认 Waveshare 1.8 AMOLED 图标网格 UI
.\pio run -e rakos_os_appui -t upload

# PY206-W38-V2 + ESP32-S3-DevKitC-1 N16R8
.\pio run -e rakos_os_py206 -t upload

# Waveshare LCD-5
.\pio run -e rakos_os_lcd5_appui -t upload
```

OS 固件运行在 `ota_0`。完整 factory 镜像包含 bootloader、分区表和 OS，可从 `0x0` 烧录。

## 首次使用

1. 按目标硬件选择对应 env 构建并烧录。
2. 打开串口监视器，确认屏幕和触摸初始化日志。
3. 进入 RAKOS 的 `Setup` 页面配置 WiFi/BLE。
4. 有 SD 卡的设备可把 `Games/<App>/app.bin` 放到 TF 卡根目录，在 `Apps` 页面扫描和安装。

**免编译应用包**：预编译备份见 [dist/firmware_backup/apps-0.2.0-dev-20260627/](dist/firmware_backup/apps-0.2.0-dev-20260627/README.zh-CN.md)（Memo / Clock / Btn / Meter / Slider）。

无 SD 卡的设备（如当前 PY206 飞线版本）仍可使用 Setup 页面保存 WiFi 到 NVS。

## 常用文档

- [快速使用](docs/quick-start.zh-CN.md)
- [已支持设备与构建环境](docs/boards.zh-CN.md)
- [PY206-W38-V2 硬件适配](docs/hardware-py206.zh-CN.md)
- [WiFi 配置](docs/wifi.zh-CN.md)
- [用户应用与 SD 包](docs/apps.zh-CN.md)
- [快速适配新设备](docs/porting-board.zh-CN.md)
- [设计与架构](docs/design.zh-CN.md)

## 项目结构

```text
src/                  RAKOS 主固件与 LVGL UI
lib/rakos_bsp/        显示、触摸、输入、存储、无线等 BSP 服务
lib/rakos_core/       启动管理、系统配置
lib/rakos_apps/       应用扫描、安装与注册
lib/rakos_app_policy/ 用户应用回滚策略
examples/             示例用户应用
docs/                 使用、适配、设计和硬件文档
```

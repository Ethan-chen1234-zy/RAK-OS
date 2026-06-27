# 用户应用与 SD 包

RAKOS 的用户应用运行在 `ota_1`，主系统运行在 `ota_0`。应用可以通过 SD 卡安装，也可以在开发阶段直接烧录到 `0x400000`。

## SD 包结构

TF 卡根目录：

```text
Games/Hello/app.bin
Games/Clock/app.bin
Games/Btn/app.bin
```

RAKOS 的 `Apps` 页面点击 `SD Scan` 后会扫描分类目录，并将应用写入 `ota_1`。

## 路径差异

- Waveshare 1.8 AMOLED：VFS 根通常为 `/`，路径形如 `/Games/Hello/app.bin`
- LCD-5：通常为 `/sdcard`，路径形如 `/sdcard/Games/Hello/app.bin`
- PY206 当前飞线版本：未接 SD，不能使用 SD 应用包

## 示例应用

| 应用 | AMOLED Env | LCD-5 Env | 说明 |
|------|------------|-----------|------|
| Memo (Hello) | `hello_app` | `hello_app_lcd5` | SD 笔记 `/notes/memo.txt`，Load/Save |
| Clock | `clock_app` | `clock_app_lcd5` | NTP + RTC + 秒表；需 SD 根目录 `wifi.ini` 才能 NTP |
| Btn | `btn_app` | 暂无 | 计时器、ota 信息、按键 beep |
| Meter | `meter_app` | 暂无 | IMU 倾角表，点表盘切换演示模式 |
| Slider | `slider_app` | 暂无 | 真实亮度/音量（与 OS 共用 NVS） |

**预编译备份**（免编译装 SD）：[../dist/firmware_backup/apps-0.2.0-dev-20260627/](../dist/firmware_backup/apps-0.2.0-dev-20260627/README.zh-CN.md)

一键构建 AMOLED 示例：

```powershell
.\pio run -e hello_app -e clock_app -e btn_app -e meter_app -e slider_app
```

一键构建 LCD-5 示例：

```powershell
.\pio run -e hello_app_lcd5 -e clock_app_lcd5
```

## 启动确认与回退

应用通过 `AppRuntime::pumpUi(display, app_ready_ms)` 在运行约 3 秒后确认 `ota_1` 启动；若 App 崩溃，Bootloader 回滚到 `ota_0`，OS 会提示 “App rolled back”。

OS 侧安装使用 `AppInstaller`（CRC 跳过相同镜像、失败强制重装）。详见 [CHANGELOG.md](../CHANGELOG.md)。

## 返回 RAKOS

- 有 BOOT 按键的设备：长按 BOOT 返回 RAKOS
- 无 BOOT 按键的设备：应用内应提供屏幕按钮，例如 `RAKOS`

## 开发阶段直接烧录

```powershell
.\pio run -e hello_app -t upload --upload-port COM5
```

## 注意事项

全片擦除或重新烧录 factory 镜像会清空 `ota_1`，需要重新安装应用。

如果新设备没有 SD 卡，建议先完成主系统、屏幕、触摸和 WiFi，再考虑增加其他应用分发方式。

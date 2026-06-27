# 快速使用

本文只覆盖从源码构建、烧录、启动验证到首次配置的最短路径。硬件细节见 [已支持设备与构建环境](boards.zh-CN.md)。

## 准备

- ESP32-S3 目标设备
- USB 数据线
- PlatformIO / pioarduino 环境
- Windows 下建议使用项目自带 `pio` 包装脚本

```powershell
.\pio run
```

如果需要指定串口：

```powershell
.\pio run -e rakos_os_appui -t upload --upload-port COM5
.\pio device monitor -b 115200 --port COM5
```

## 选择构建环境

常用 OS 固件：

```powershell
# Waveshare 1.8 AMOLED，默认图标网格 UI
.\pio run -e rakos_os_appui -t upload

# ESP32-S3-DevKitC-1 N16R8 + PY206-W38-V2
.\pio run -e rakos_os_py206 -t upload

# Waveshare LCD-5
.\pio run -e rakos_os_lcd5_appui -t upload
```

## 启动检查

烧录后打开串口监视器：

```powershell
.\pio device monitor -b 115200
```

重点看这些日志：

```text
[BSP] Display bring-up ...
[BSP] ... panel up
[BSP] ... touch ready ...
[OK] FreeRTOS scheduler running
```

如果屏幕不亮，先确认构建 env 是否匹配硬件。  
如果触摸无效，先确认触摸芯片、I2C 地址、SDA/SCL、INT/RST 和串口日志。

## 首次配置

进入 RAKOS 的 `Setup` 页面：

- `Enable WiFi`：开启 WiFi
- `SSID` / `Password`：输入网络信息
- `Save WiFi / BLE`：保存到 NVS

有 SD 卡时，WiFi 可同步到 `wifi.ini`。无 SD 卡时，NVS 配置仍然可用。

## 用户应用

有 SD 卡的设备，把应用放到 TF 卡根目录：

```text
Games/Hello/app.bin
Games/Clock/app.bin
```

进入 `Apps` 页面，点击 `SD Scan` 后安装或启动应用。更多说明见 [用户应用与 SD 包](apps.zh-CN.md)。

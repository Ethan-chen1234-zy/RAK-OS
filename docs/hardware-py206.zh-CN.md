# PY206-W38-V2 硬件适配

本文记录 `ESP32-S3-DevKitC-1 N16R8 + PY206-W38-V2` 转接屏版本的接线、构建环境和注意事项。

## 构建环境

```powershell
.\pio run -e rakos_os_py206 -t upload
.\pio device monitor -b 115200
```

对应宏：

```cpp
RAKOS_BOARD_PY206_W38_V2
RAKOS_PANEL_CO5300
```

## 接线

| 转接板 | ESP32-S3 GPIO | 说明 |
|--------|---------------|------|
| GND | GND | 共地 |
| 3V3 | 3V3 | 不要接 5V |
| CLK | GPIO11 | QSPI SCLK |
| SIO0 | GPIO4 | QSPI D0 |
| SIO1 | GPIO5 | QSPI D1 |
| SIO2 | GPIO6 | QSPI D2 |
| SIO3 | GPIO7 | QSPI D3 |
| CS | GPIO12 | QSPI CS |
| RST | GPIO8 | LCD reset |
| TE | 不接 | 当前未使用 |
| SCL | GPIO14 | 触摸 I2C SCL |
| SDA | GPIO15 | 触摸 I2C SDA |
| INT | GPIO38 | 触摸中断 |
| 最右 RST | GPIO9 | 触摸 reset |

注意两个 `RST` 不是同一个信号：`CS` 后面的 `RST` 是屏幕复位，最右侧 `RST` 是触摸复位。

## 屏幕与触摸

- 屏幕控制器：`CO5300`
- 分辨率：`410x502`
- 触摸芯片：`CST92xx`
- 触摸 I2C 地址：`0x5A`
- AMOLED 四角安全边距：`UI_SAFE_INSET = 20`

启动日志中应看到类似：

```text
[BSP] Display bring-up (PY206-W38-V2 CO5300 AMOLED)...
[BSP] CO5300 panel up
[BSP] CST9220 touch ready @0x5A
```

如果触摸初始化失败，先看 I2C scan 是否能扫到 `0x5A`，再检查 `SDA/SCL/INT/RST` 接线。

## DevKitC USB 使用

- `USB-to-UART` 口：推荐用于烧录和串口日志
- `USB` 口：ESP32-S3 原生 USB，可用于 USB CDC、MSC、HID 等功能

两个 USB 口可以同时插，一个用于日志和烧录，一个用于原生 USB 功能测试。

## 当前限制

- 当前飞线版本未接 SD 卡，应用安装和 `wifi.ini` 不可用
- WiFi 使用 `Setup` 页面保存到 NVS
- GPIO38 同时是 DevKitC 板载 RGB LED 和触摸 INT，当前优先作为触摸中断使用

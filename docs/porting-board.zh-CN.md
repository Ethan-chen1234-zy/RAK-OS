# 快速适配新设备

新增设备时，目标是把硬件差异限制在 BSP 层：新增 env、新增 board 宏、新增引脚配置，尽量不改 UI 和业务逻辑。

## 推荐顺序

1. 串口启动
2. 屏幕点亮
3. 触摸可用
4. WiFi/NVS 可用
5. SD、音频、IMU、电源管理等外设
6. 用户应用适配

不要一开始就同时接入所有外设。先拿到显示和触摸的最小闭环。

## 1. 新增 PlatformIO env

在 `platformio.ini` 中新增环境：

```ini
[env:rakos_os_xxx]
extends = env:rakos_os_local
custom_prog_name = RAKOS-XXX
build_flags =
  ${env:rakos_os_local.build_flags}
  -D RAKOS_BOARD_XXX=1
  -D RAKOS_UI_APP_GRID=1
```

如果芯片、Flash、PSRAM 与现有 `esp32-s3-devkitc1-n16r8` 不一致，再新增或修改 board json。

## 2. 新增引脚配置

在 `lib/rakos_bsp/include/rakos/pin_config.h` 增加分支：

```cpp
#elif defined(RAKOS_BOARD_XXX)

#define RAKOS_HAS_BOOT_BUTTON 1
#define RAKOS_HAS_PWR_BUTTON  0
#define RAKOS_HAS_IO_EXPANDER 0
#define RAKOS_HAS_SD_CARD     0
#define RAKOS_HAS_AUDIO       0
#define RAKOS_HAS_IMU         0

#define LCD_WIDTH             410
#define LCD_HEIGHT            502
#define UI_SAFE_INSET         20

#define LCD_CS                12
#define LCD_SCLK              11
#define LCD_SDIO0             4
#define LCD_SDIO1             5
#define LCD_SDIO2             6
#define LCD_SDIO3             7
#define LCD_RST               8

#define I2C_SDA               15
#define I2C_SCL               14
#define TOUCH_INT             38
#define TOUCH_RST             9
```

能力宏用于让存储、输入、UI 等服务自动启用或降级。

## 3. 接入屏幕

如果是已有控制器，优先在 `DisplayManager` 里通过宏选择已有驱动：

```cpp
#define RAKOS_PANEL_CO5300 1
```

如果是全新控制器，先确认：

- 总线类型：QSPI、SPI、RGB、MIPI 等
- 分辨率
- reset / backlight / TE 是否需要
- 像素格式和字节序
- 是否有坐标偏移

屏幕点亮后再处理触摸。

## 4. 接入触摸

确认触摸芯片：

- I2C 地址
- SDA/SCL
- INT/RST
- 坐标范围
- 是否需要 swap/mirror

推荐优先使用现有库：`Arduino_DriveBus`、`SensorLib`、`ESP32_Display_Panel`。不要为每块板复制一套完整 UI。

## 5. 外设逐个启用

常见外设：

- SD：确认 SPI/SDMMC、CS、挂载点
- WiFi：NVS Setup 优先，SD `wifi.ini` 可选
- 音频：I2S MCLK/BCLK/WS/DOUT/DIN
- IMU：I2C 地址和中断
- IO 扩展器：型号、地址、复位和背光脚

每启用一个外设，都应保证没有该外设时系统仍可启动。

## 6. 验证

至少验证：

```powershell
.\pio run -e rakos_os_xxx
.\pio run -e rakos_os_xxx -t upload
.\pio device monitor -b 115200
```

启动日志应能确认：

```text
Display bring-up
panel up
touch ready
FreeRTOS scheduler running
```

## 文档要求

每新增一块设备，应补一篇：

```text
docs/hardware-xxx.zh-CN.md
```

内容包括接线、env、屏幕、触摸、外设能力、限制和排障日志。

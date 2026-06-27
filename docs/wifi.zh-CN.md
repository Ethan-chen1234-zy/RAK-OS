# WiFi 配置

RAKOS 支持两种 WiFi 配置来源：

- `Setup` 页面保存到 NVS
- SD 卡根目录 `wifi.ini`

无 SD 卡设备应使用 `Setup` 页面。

## Setup 页面配置

适用于所有设备，尤其是 PY206 当前无 SD 版本。

步骤：

1. 进入 RAKOS 的 `Setup` 页面。
2. 打开 `Enable WiFi`。
3. 输入 `SSID` 和 `Password`。
4. 点击 `Save WiFi / BLE`。
5. 回到 Home 页观察 WiFi 状态。

配置会保存到 ESP32 flash 的 NVS 中，重启后仍然保留。

串口成功日志示例：

```text
[RADIO] Scanning for "YourSSID"...
[RADIO] Connecting to "YourSSID"...
[RADIO] WiFi connected, IP=192.168.x.x
[RADIO] NTP sync started (UTC+8)
```

## SD 卡 wifi.ini

有 SD 卡的设备可在 TF 卡根目录放置：

```ini
ssid=YourSSID
password=YourPassword
```

路径：

```text
wifi.ini
Games/Hello/app.bin
```

启动后 RAKOS 会尝试读取 `wifi.ini`，并覆盖当前 NVS WiFi 字段。

## 优先级

RAKOS 主系统：

1. SD 挂载且存在 `wifi.ini` 时，使用 `wifi.ini`
2. 否则使用 Setup 页面保存的 NVS 配置
3. 如果 WiFi 未启用或 SSID 为空，则保持离线

部分独立用户 App 可能仍只读 SD `wifi.ini`。如果需要所有 App 复用 Setup 配置，应将 App 的 WiFi/时间服务改为优先读取 NVS。

## 常见问题

`WiFi failed`：检查 SSID 是否在扫描列表中、密码是否正确、路由器是否支持 2.4GHz。  
`No wifi.ini`：这只表示 SD 配置不存在，不影响 NVS 手动配置。  
无 SD 卡：直接使用 Setup 页面即可。  
时间不显示：等待 NTP 同步完成，或确认 WiFi 已连接互联网。

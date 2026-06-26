# ClockApp — RAKOS watch demo

Analog + digital clock for **ota_1** (`0x400000`). Reads the board RTC (PCF85063 / PCF8563 on I2C) when available; otherwise runs a software clock seeded from firmware build time.

## Build

From the **RAK_os repo root**:

```bash
pio run -e clock_app
```

Output:

- `.pio/build/clock_app/Clock.bin` — raw firmware
- `examples/ClockApp/dist/app.bin` — SD card package

## Test via SD card

1. Flash **RAKOS** OS: `pio run -e rakos_os_local -t upload`
2. Copy `examples/ClockApp/dist/app.bin` to microSD **root** (on PC):

   ```text
   Games/Clock/app.bin
   ```

   Device path is `/sdcard/Games/Clock/app.bin` — do **not** create an extra `sdcard` folder on the card.

3. Boot RAKOS → **Apps** → **Refresh SD apps** → tap **Games / Clock**

## Test via USB

```bash
pio run -e clock_app -t upload
```

Then in RAKOS: **Apps** → **USB / ota_1 (flashed)**.

## Return to RAKOS

- **BOOT** short press, or
- **PWR** hold ~1.2 s

## UI

- Large digital `HH:MM:SS`
- Analog dial with hour / minute / second hands
- Date line and RTC vs soft-clock indicator

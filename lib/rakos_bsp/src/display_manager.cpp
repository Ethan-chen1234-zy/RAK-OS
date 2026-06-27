#if !defined(RAKOS_BOARD_WAVESHARE_LCD5) && !defined(RAKOS_BOARD_WAVESHARE_LCD5B)

#include <rakos/display_manager.h>

#include <rakos/io_expander.h>
#include <rakos/i2c_bus.h>
#include <Preferences.h>
#include <esp_heap_caps.h>
#include <esp_timer.h>

DisplayManager *DisplayManager::instance = nullptr;

static Preferences prefs;
static Arduino_IIC *g_touch_for_isr = nullptr;
static esp_timer_handle_t g_lvgl_tick_timer = nullptr;
static int16_t g_touch_last_x = 0;
static int16_t g_touch_last_y = 0;
static bool g_touch_down = false;

static void lvgl_tick_timer_cb(void *arg) {
    (void)arg;
    lv_tick_inc(2);
}

static void touch_interrupt_handler() {
    if (g_touch_for_isr) {
        g_touch_for_isr->IIC_Interrupt_Flag = true;
    }
}

void DisplayManager::display_rounder_event_cb(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_INVALIDATE_AREA) {
        return;
    }
    lv_area_t *area = lv_event_get_invalidated_area(e);
    if (!area) {
        return;
    }
    area->x1 &= ~1;
    area->y1 &= ~1;
    area->x2 = (area->x2 & ~1) + 1;
    area->y2 = (area->y2 & ~1) + 1;
}

DisplayManager::DisplayManager()
    : bus(nullptr),
      gfx(nullptr),
#if defined(RAKOS_BOARD_PY206_W38_V2)
      touch_cst92xx_(nullptr),
#else
      touch_bus_(nullptr),
      touch_(nullptr),
#endif
      display(nullptr),
      buf(nullptr),
      buf2(nullptr) {
    instance = this;
}

DisplayManager::~DisplayManager() {
    g_touch_for_isr = nullptr;
#if defined(RAKOS_BOARD_PY206_W38_V2)
    touch_cst92xx_.reset();
#else
    touch_.reset();
    touch_bus_.reset();
#endif
    if (buf) {
        free(buf);
    }
    if (buf2) {
        free(buf2);
    }
    delete gfx;
    delete bus;
    instance = nullptr;
}

bool DisplayManager::initTouch() {
    g_touch_for_isr = nullptr;
#if defined(RAKOS_BOARD_PY206_W38_V2)
    touch_cst92xx_.reset();
#else
    touch_.reset();
    touch_bus_.reset();
#endif

#if RAKOS_HAS_IO_EXPANDER
    auto &expander = rakos::IoExpander::instance();
    if (expander.ready()) {
        expander.pulseTouchReset();
    }
#elif TOUCH_RST >= 0
    pinMode(TOUCH_RST, OUTPUT);
    digitalWrite(TOUCH_RST, LOW);
    delay(20);
    digitalWrite(TOUCH_RST, HIGH);
    delay(120);
#endif

    rakos::i2cBusRecover();
    delay(50);
    pinMode(TOUCH_INT, INPUT_PULLUP);

#if defined(RAKOS_BOARD_PY206_W38_V2)
    rakos::I2cLockGuard bus_lock(500);
    if (!bus_lock.locked()) {
        Serial.println("[BSP] CST92xx init: I2C lock timeout");
        return false;
    }

    touch_cst92xx_ = std::make_unique<TouchDrvCST92xx>();
    touch_cst92xx_->setPins(TOUCH_RST, TOUCH_INT);
    touch_cst92xx_->setMaxCoordinates(LCD_WIDTH, LCD_HEIGHT);
    touch_cst92xx_->setMirrorXY(false, false);
    touch_cst92xx_->setSwapXY(false);

    for (uint8_t attempt = 0; attempt < 5; ++attempt) {
        if (touch_cst92xx_->begin(Wire, TOUCH_I2C_ADDR, I2C_SDA, I2C_SCL)) {
            Serial.printf("[BSP] %s touch ready @0x%02X\n",
                          touch_cst92xx_->getModelName(),
                          TOUCH_I2C_ADDR);
            return true;
        }
        Serial.println("[BSP] CST92xx init retry...");
        delay(200);
    }

    touch_cst92xx_.reset();
    Serial.println("[BSP] CST92xx touch init failed");
    rakos::i2cBusScan();
    return false;
#else
    rakos::I2cLockGuard bus_lock(500);
    if (!bus_lock.locked()) {
        Serial.println("[BSP] Touch init: I2C lock timeout");
        return false;
    }

    touch_bus_ = std::make_shared<Arduino_HWIIC>(I2C_SDA, I2C_SCL, &Wire);

    auto tryBegin = [this]() -> bool {
        for (uint8_t attempt = 0; attempt < 5; ++attempt) {
            if (touch_->begin()) {
                return true;
            }
            Serial.println("[BSP] Touch init retry...");
            delay(200);
        }
        return false;
    };

    // Waveshare Arduino demos call begin() directly (no I2C probe gate).
    touch_ = std::unique_ptr<Arduino_IIC>(new Arduino_FT3x68(
        touch_bus_, FT3168_DEVICE_ADDRESS, DRIVEBUS_DEFAULT_VALUE, TOUCH_INT, touch_interrupt_handler));
    g_touch_for_isr = touch_.get();
    if (tryBegin()) {
        touch_->IIC_Write_Device_State(
            touch_->Arduino_IIC_Touch::Device::TOUCH_POWER_MODE,
            touch_->Arduino_IIC_Touch::Device_Mode::TOUCH_POWER_MONITOR);
        Serial.println("[BSP] FT3168 ready");
        return true;
    }
    g_touch_for_isr = nullptr;
    touch_.reset();

    touch_ = std::unique_ptr<Arduino_IIC>(new Arduino_CST816x(
        touch_bus_, TOUCH_I2C_ADDR, DRIVEBUS_DEFAULT_VALUE, TOUCH_INT, touch_interrupt_handler));
    g_touch_for_isr = touch_.get();
    if (tryBegin()) {
        touch_->IIC_Write_Device_State(
            touch_->Arduino_IIC_Touch::Device::TOUCH_DEVICE_INTERRUPT_MODE,
            touch_->Arduino_IIC_Touch::Device_Mode::TOUCH_DEVICE_INTERRUPT_PERIODIC);
        Serial.println("[BSP] CST816 ready");
        return true;
    }
    g_touch_for_isr = nullptr;
    touch_.reset();
    touch_bus_.reset();

    Serial.println("[BSP] Touch init failed");
    rakos::i2cBusScan();
    return false;
#endif
}

bool DisplayManager::readTouch(int16_t &x, int16_t &y) {
#if defined(RAKOS_BOARD_PY206_W38_V2)
    if (!touch_cst92xx_) {
        return false;
    }

    const TouchPoints &points = touch_cst92xx_->getTouchPoints();
    if (!points.hasPoints()) {
        return false;
    }

    const TouchPoint &point = points.getPoint(0);
    x = (int16_t)point.x;
    y = (int16_t)point.y;
    return true;
#else
    if (!touch_) {
        return false;
    }

    x = (int16_t)touch_->IIC_Read_Device_Value(
        touch_->Arduino_IIC_Touch::Value_Information::TOUCH_COORDINATE_X);
    y = (int16_t)touch_->IIC_Read_Device_Value(
        touch_->Arduino_IIC_Touch::Value_Information::TOUCH_COORDINATE_Y);
    return touch_->IIC_Interrupt_Flag;
#endif
}

bool DisplayManager::init() {
#if defined(RAKOS_BOARD_PY206_W38_V2)
    Serial.println("[BSP] Display bring-up (PY206-W38-V2 CO5300 AMOLED)...");
#else
    Serial.println("[BSP] Display bring-up (Waveshare 1.8 AMOLED)...");
#endif

    prefs.begin("rakos_ui", false);

#if RAKOS_HAS_IO_EXPANDER
    auto &expander = rakos::IoExpander::instance();
    if (!expander.ready() && !expander.begin()) {
        Serial.println("[BSP] IO expander init failed");
        return false;
    }
#endif

    bus = new Arduino_ESP32QSPI(LCD_CS, LCD_SCLK, LCD_SDIO0, LCD_SDIO1, LCD_SDIO2, LCD_SDIO3);
#if defined(RAKOS_PANEL_CO5300)
    gfx = new Arduino_CO5300(
        bus, LCD_RST, 0, LCD_WIDTH, LCD_HEIGHT, LCD_COL_OFFSET, LCD_ROW_OFFSET);
#else
    gfx = new Arduino_SH8601(bus, GFX_NOT_DEFINED, 0, LCD_WIDTH, LCD_HEIGHT);
#endif
    if (!gfx->begin()) {
        Serial.println("[BSP] Panel init failed");
        return false;
    }

    gfx->setRotation(0);
    {
        const uint8_t pct = getBrightnessPercentage();
        const uint8_t level = (uint8_t)(((uint16_t)pct * 255 + 50) / 100);
        gfx->setBrightness(level);
    }
    gfx->fillScreen(0x0010);
    delay(80);
    gfx->fillScreen(0x0000);
#if defined(RAKOS_PANEL_CO5300)
    Serial.println("[BSP] CO5300 panel up");
#else
    Serial.println("[BSP] SH8601 panel up");
#endif

    lv_init();

    const size_t line_height = LCD_DRAW_BUFF_HEIGHT;
    size_t draw_buf_pixels = (size_t)LCD_WIDTH * line_height;
    size_t draw_buf_bytes = draw_buf_pixels * sizeof(lv_color_t);

    buf = (lv_color_t *)heap_caps_malloc(draw_buf_bytes, MALLOC_CAP_DMA);
    if (!buf) {
        buf = (lv_color_t *)heap_caps_malloc(draw_buf_bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    }
    if (!buf) {
        Serial.println("[BSP] LVGL buffer alloc failed");
        return false;
    }

    buf2 = (lv_color_t *)heap_caps_malloc(draw_buf_bytes, MALLOC_CAP_DMA);
    if (!buf2) {
        buf2 = (lv_color_t *)heap_caps_malloc(draw_buf_bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    }

    display = lv_display_create(LCD_WIDTH, LCD_HEIGHT);
    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
    lv_display_set_flush_cb(display, disp_flush_callback);
    lv_display_add_event_cb(display, display_rounder_event_cb, LV_EVENT_INVALIDATE_AREA, nullptr);
    lv_display_set_buffers(display, buf, buf2, (uint32_t)draw_buf_bytes, LV_DISPLAY_RENDER_MODE_PARTIAL);

    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, touchpad_read_callback);

    Serial.println("[BSP] Display ready");
    return true;
}

void DisplayManager::startLvglTick() {
    if (g_lvgl_tick_timer) {
        return;
    }
    const esp_timer_create_args_t args = {
        .callback = &lvgl_tick_timer_cb,
        .arg = nullptr,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "lvgl_tick",
        .skip_unhandled_events = false,
    };
    if (esp_timer_create(&args, &g_lvgl_tick_timer) == ESP_OK) {
        esp_timer_start_periodic(g_lvgl_tick_timer, 2000);
        Serial.println("[BSP] LVGL tick timer started");
    }
}

void DisplayManager::runLvgl() {
    lv_timer_handler();
}

void DisplayManager::update() {
    runLvgl();
}

void DisplayManager::forceRefresh() {
    if (display) {
        lv_refr_now(display);
    }
}

void DisplayManager::setBrightness(uint8_t brightness, bool persist) {
    if (gfx) {
        gfx->setBrightness(brightness);
    }
    if (!persist) {
        return;
    }
    uint8_t pct = (uint8_t)(((uint16_t)brightness * 100 + 127) / 255);
    if (pct > 100) {
        pct = 100;
    }
    prefs.putUChar("brightness_pct", pct);
}

void DisplayManager::setBrightnessPercent(uint8_t pct, bool persist) {
    if (pct > 100) {
        pct = 100;
    }
    const uint8_t level = (uint8_t)(((uint16_t)pct * 255 + 50) / 100);
    setBrightness(level, persist);
}

uint8_t DisplayManager::getBrightnessPercentage() {
    uint8_t pct = prefs.getUChar("brightness_pct", 80);
    if (pct == 0) {
        pct = 80;
        prefs.putUChar("brightness_pct", 80);
    }
    return pct > 100 ? 100 : pct;
}

bool DisplayManager::getTouchCoordinates(int16_t &x, int16_t &y) {
#if defined(RAKOS_BOARD_PY206_W38_V2)
    rakos::I2cLockGuard lock(100);
    if (!lock.locked()) {
        return false;
    }
    return readTouch(x, y);
#else
    if (!touch_ || !touch_->IIC_Interrupt_Flag) {
        return false;
    }
    touch_->IIC_Interrupt_Flag = false;
    x = (int16_t)touch_->IIC_Read_Device_Value(
        touch_->Arduino_IIC_Touch::Value_Information::TOUCH_COORDINATE_X);
    y = (int16_t)touch_->IIC_Read_Device_Value(
        touch_->Arduino_IIC_Touch::Value_Information::TOUCH_COORDINATE_Y);
    return true;
#endif
}

void DisplayManager::disp_flush_callback(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    if (!instance || !instance->gfx) {
        lv_display_flush_ready(disp);
        return;
    }
    const uint32_t w = area->x2 - area->x1 + 1;
    const uint32_t h = area->y2 - area->y1 + 1;
#if LV_COLOR_16_SWAP
    instance->gfx->draw16bitBeRGBBitmap(area->x1, area->y1, (uint16_t *)px_map, w, h);
#else
    instance->gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)px_map, w, h);
#endif
    lv_display_flush_ready(disp);
}

void DisplayManager::touchpad_read_callback(lv_indev_t *indev, lv_indev_data_t *data) {
    (void)indev;
    data->point.x = g_touch_last_x;
    data->point.y = g_touch_last_y;
    data->state = g_touch_down ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;

    if (!instance ||
#if defined(RAKOS_BOARD_PY206_W38_V2)
        !instance->touch_cst92xx_
#else
        !instance->touch_
#endif
    ) {
        g_touch_down = false;
        return;
    }

#if defined(RAKOS_BOARD_PY206_W38_V2)
    rakos::I2cLockGuard lock(100);
    if (!lock.locked()) {
        if (g_touch_down) {
            data->state = LV_INDEV_STATE_PRESSED;
        }
        return;
    }

    int16_t x = 0;
    int16_t y = 0;
    if (instance->readTouch(x, y) && x >= 0 && y >= 0 && x < LCD_WIDTH && y < LCD_HEIGHT) {
        g_touch_last_x = x;
        g_touch_last_y = y;
        g_touch_down = true;
        data->state = LV_INDEV_STATE_PRESSED;
        data->point.x = x;
        data->point.y = y;
    } else {
        g_touch_down = false;
        data->state = LV_INDEV_STATE_RELEASED;
    }
#else
    const bool active =
        instance->touch_->IIC_Interrupt_Flag || digitalRead(TOUCH_INT) == LOW;
    if (!active) {
        g_touch_down = false;
        data->state = LV_INDEV_STATE_RELEASED;
        return;
    }

    rakos::I2cLockGuard lock(100);
    if (!lock.locked()) {
        if (g_touch_down) {
            data->state = LV_INDEV_STATE_PRESSED;
        }
        return;
    }

    const int16_t x = (int16_t)instance->touch_->IIC_Read_Device_Value(
        instance->touch_->Arduino_IIC_Touch::Value_Information::TOUCH_COORDINATE_X);
    const int16_t y = (int16_t)instance->touch_->IIC_Read_Device_Value(
        instance->touch_->Arduino_IIC_Touch::Value_Information::TOUCH_COORDINATE_Y);

    instance->touch_->IIC_Interrupt_Flag = false;
    if (x >= 0 && y >= 0 && x < LCD_WIDTH && y < LCD_HEIGHT) {
        g_touch_last_x = x;
        g_touch_last_y = y;
        g_touch_down = true;
        data->state = LV_INDEV_STATE_PRESSED;
        data->point.x = x;
        data->point.y = y;
    } else {
        g_touch_down = false;
        data->state = LV_INDEV_STATE_RELEASED;
    }
#endif
}

#endif /* !RAKOS_BOARD_WAVESHARE_LCD5 */

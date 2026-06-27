/**
 * RAKOS Memo — simple SD notes (ota_1 @ 0x400000)
 *
 * Saves to /notes/memo.txt on TF card (same mount as RAKOS apps).
 */
#include <Arduino.h>
#include <lvgl.h>
#include <FS.h>

#include <rakos/app_runtime.h>
#include <rakos/boot_manager.h>
#include <rakos/display_manager.h>
#include <rakos/input_manager.h>
#include <rakos/lvgl_compat.h>
#include <rakos/pin_config.h>
#include <rakos/sd_fs.h>
#include <rakos/storage_service.h>

static DisplayManager display;
static InputManager input;
static StorageService storage;

static lv_obj_t *status_label = nullptr;
static lv_obj_t *editor = nullptr;
static uint32_t app_ready_ms = 0;
static bool sd_ok = false;

static constexpr uint32_t kExitGraceMs = 3000;
static constexpr uint32_t kExitBootHoldMs = 1500;
static constexpr const char *kMemoPath = "/notes/memo.txt";

static void setStatus(const char *text) {
    if (status_label) {
        lv_label_set_text(status_label, text);
    }
}

static bool ensureNotesDir() {
    fs::FS &fs = rakos::sdFs();
    File d = fs.open("/notes", FILE_READ);
    if (d && d.isDirectory()) {
        d.close();
        return true;
    }
    if (d) {
        d.close();
    }
    return fs.mkdir("/notes");
}

static bool loadMemo() {
    if (!editor || !sd_ok) {
        return false;
    }
    fs::FS &fs = rakos::sdFs();
    File f = fs.open(kMemoPath, FILE_READ);
    if (!f) {
        lv_textarea_set_text(editor, "Hello RAKOS!\nEdit and tap Save.");
        return false;
    }
    String text = f.readString();
    f.close();
    if (text.isEmpty()) {
        text = " ";
    }
    lv_textarea_set_text(editor, text.c_str());
    return true;
}

static bool saveMemo() {
    if (!editor || !sd_ok) {
        return false;
    }
    if (!ensureNotesDir()) {
        return false;
    }
    fs::FS &fs = rakos::sdFs();
    File f = fs.open(kMemoPath, FILE_WRITE);
    if (!f) {
        return false;
    }
    const char *text = lv_textarea_get_text(editor);
    f.print(text ? text : "");
    f.close();
    return true;
}

static void on_save(lv_event_t *e) {
    (void)e;
    if (!sd_ok) {
        setStatus("SD not mounted");
        return;
    }
    setStatus(saveMemo() ? "Saved to /notes/memo.txt" : "Save failed");
}

static void on_load(lv_event_t *e) {
    (void)e;
    if (!sd_ok) {
        setStatus("SD not mounted");
        return;
    }
    setStatus(loadMemo() ? "Loaded from SD" : "New note (file missing)");
}

static void build_ui() {
    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x001018), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "Memo");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFF7F1F), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 28);

    editor = lv_textarea_create(scr);
    lv_obj_set_size(editor, lv_pct(92), 260);
    lv_obj_align(editor, LV_ALIGN_TOP_MID, 0, 68);
    lv_textarea_set_max_length(editor, 2048);
    lv_textarea_set_one_line(editor, false);
    lv_obj_set_style_bg_color(editor, lv_color_hex(0x0A1820), 0);
    lv_obj_set_style_border_color(editor, lv_color_hex(0x333333), 0);
    lv_obj_set_style_text_color(editor, lv_color_hex(0xE8E4E0), 0);

    lv_obj_t *row = lv_obj_create(scr);
    lv_obj_remove_style_all(row);
    lv_obj_set_size(row, lv_pct(92), 44);
    lv_obj_align(row, LV_ALIGN_TOP_MID, 0, 340);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *btn_load = lv_button_create(row);
    lv_obj_set_size(btn_load, 120, 40);
    lv_obj_add_event_cb(btn_load, on_load, LV_EVENT_CLICKED, nullptr);
    lv_obj_t *lbl_load = lv_label_create(btn_load);
    lv_label_set_text(lbl_load, "Load");
    lv_obj_center(lbl_load);

    lv_obj_t *btn_save = lv_button_create(row);
    lv_obj_set_size(btn_save, 120, 40);
    lv_obj_set_style_bg_color(btn_save, lv_color_hex(0xFF7F1F), 0);
    lv_obj_add_event_cb(btn_save, on_save, LV_EVENT_CLICKED, nullptr);
    lv_obj_t *lbl_save = lv_label_create(btn_save);
    lv_label_set_text(lbl_save, "Save");
    lv_obj_center(lbl_save);

    status_label = lv_label_create(scr);
    lv_label_set_text(status_label, sd_ok ? "Ready" : "Insert SD for save/load");
    lv_obj_set_style_text_color(status_label, lv_color_hex(0x9A948F), 0);
    lv_obj_align(status_label, LV_ALIGN_BOTTOM_MID, 0, -72);

    rakos::AppRuntime::addExitButton(scr);
}

void setup() {
    Serial.begin(115200);
    delay(300);
    Serial.println();
    Serial.println("=== Memo (RAKOS demo, ota_1) ===");

    BootManager::begin();
    if (!BootManager::isRunningAppSlot()) {
        Serial.println("[WARN] Not running from ota_1");
    }

    if (!rakos::AppRuntime::beginHardware(display, input)) {
        Serial.println("[FATAL] Display init failed");
        for (;;) {
            delay(1000);
        }
    }

    sd_ok = storage.init() && storage.sdReady();
    if (!sd_ok) {
        Serial.println("[MEMO] SD not ready — edit only until card inserted");
    }

    rakos::AppRuntime::buildUiLocked(build_ui);

    if (sd_ok) {
        loadMemo();
        setStatus("Loaded /notes/memo.txt or new note");
    }

    app_ready_ms = millis();
    Serial.println("[OK] Memo running");
}

void loop() {
    input.update();

    if (rakos::AppRuntime::bootExitRequested(input, app_ready_ms, kExitGraceMs, kExitBootHoldMs)) {
        if (BootManager::setNextBootOs()) {
            BootManager::reboot();
        }
    }

    rakos::AppRuntime::pumpUi(display, app_ready_ms);
}

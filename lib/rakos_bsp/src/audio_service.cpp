#include <rakos/audio_service.h>
#include <rakos/pin_config.h>

#if RAKOS_HAS_AUDIO

#include <rakos/i2c_bus.h>
#include <Wire.h>
#include <Preferences.h>
#include <ESP_I2S.h>
#include <esp_check.h>
#include <math.h>

extern "C" {
#include "es8311.h"
}

static I2SClass g_i2s;
static es8311_handle_t g_codec = nullptr;
static Preferences g_audio_prefs;

static constexpr uint8_t kDefaultVolumePct = 75;

static float beepEnvelope(uint32_t frame, uint32_t total) {
    if (total == 0) {
        return 0.0f;
    }
    const float p = (float)frame / (float)total;
    if (p < 0.03f) {
        return p / 0.03f;
    }
    if (p > 0.72f) {
        return (1.0f - p) / 0.28f;
    }
    return 1.0f;
}

static esp_err_t init_codec(uint32_t sample_rate) {
    rakos::I2cLockGuard lock(500);
    if (!lock.locked()) {
        return ESP_ERR_TIMEOUT;
    }

    g_codec = es8311_create(0, ES8311_I2C_ADDR);
    if (!g_codec) {
        return ESP_FAIL;
    }

    const es8311_clock_config_t clk = {
        .mclk_inverted = false,
        .sclk_inverted = false,
        .mclk_from_mclk_pin = true,
        .mclk_frequency = (int)(sample_rate * 256),
        .sample_frequency = (int)sample_rate,
    };

    ESP_RETURN_ON_ERROR(es8311_init(g_codec, &clk, ES8311_RESOLUTION_16, ES8311_RESOLUTION_16), "audio", "es8311 init");
    ESP_RETURN_ON_ERROR(es8311_sample_frequency_config(g_codec, clk.mclk_frequency, clk.sample_frequency), "audio", "es8311 rate");
    ESP_RETURN_ON_ERROR(es8311_microphone_config(g_codec, false), "audio", "es8311 mic");
    return ESP_OK;
}

static void applyCodecVolume(uint8_t volume_percent) {
    if (!g_codec) {
        return;
    }
    rakos::I2cLockGuard lock(100);
    if (!lock.locked()) {
        return;
    }
    es8311_voice_volume_set(g_codec, volume_percent, nullptr);
    es8311_microphone_gain_set(g_codec, ES8311_MIC_GAIN_24DB);
}

bool AudioService::begin(uint32_t sample_rate) {
    sample_rate_ = sample_rate;

    g_audio_prefs.begin("rakos_ui", false);
    volume_pct_ = g_audio_prefs.getUChar("audio_vol_pct", kDefaultVolumePct);
    if (volume_pct_ > 100) {
        volume_pct_ = kDefaultVolumePct;
    }

    pinMode(I2S_PA_PIN, OUTPUT);
    digitalWrite(I2S_PA_PIN, HIGH);

    g_i2s.setPins(I2S_BCLK, I2S_WS, I2S_DOUT, I2S_DIN, I2S_MCLK);
    if (!g_i2s.begin(I2S_MODE_STD, sample_rate_, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO,
                     I2S_STD_SLOT_BOTH)) {
        Serial.println("[BSP] I2S init failed");
        return false;
    }

    if (init_codec(sample_rate_) != ESP_OK) {
        Serial.println("[BSP] ES8311 init failed");
        return false;
    }

    applyCodecVolume(volume_pct_);

    ready_ = true;
    Serial.printf("[BSP] ES8311 audio ready @ %u Hz, vol=%u%%\n", sample_rate_, volume_pct_);
    return true;
}

void AudioService::setVolume(uint8_t volume_percent) {
    if (volume_percent > 100) {
        volume_percent = 100;
    }
    volume_pct_ = volume_percent;
    g_audio_prefs.putUChar("audio_vol_pct", volume_pct_);
    if (!ready_) {
        return;
    }
    applyCodecVolume(volume_pct_);
}

size_t AudioService::write(const uint8_t *data, size_t len) {
    if (!ready_) {
        return 0;
    }
    return g_i2s.write(data, len);
}

size_t AudioService::read(uint8_t *data, size_t len) {
    if (!ready_) {
        return 0;
    }
    return g_i2s.readBytes(reinterpret_cast<char *>(data), len);
}

void AudioService::beep(uint16_t freq_hz, uint16_t duration_ms) {
    if (!ready_ || freq_hz < 200 || duration_ms == 0 || volume_pct_ == 0) {
        return;
    }
    if (duration_ms > 100) {
        duration_ms = 100;
    }

    const uint32_t frames = (sample_rate_ * duration_ms) / 1000;
    if (frames == 0) {
        return;
    }

    const float amp = ((float)volume_pct_ / 100.0f) * 15000.0f;
    const float w = 2.0f * (float)M_PI * (float)freq_hz;
    const float w2 = w * 2.0f;

    int16_t buf[256 * 2];
    uint32_t written = 0;
    while (written < frames) {
        const uint32_t chunk = min(frames - written, (uint32_t)256);
        for (uint32_t i = 0; i < chunk; ++i) {
            const float t = (float)(written + i) / (float)sample_rate_;
            const float env = beepEnvelope(written + i, frames);
            const float wave = sinf(w * t) * 0.72f + sinf(w2 * t) * 0.28f;
            const int16_t sample = (int16_t)(wave * env * amp);
            buf[i * 2] = sample;
            buf[i * 2 + 1] = sample;
        }
        g_i2s.write(reinterpret_cast<uint8_t *>(buf), chunk * sizeof(int16_t) * 2);
        written += chunk;
    }
}

#else  /* !RAKOS_HAS_AUDIO */

#if defined(RAKOS_HAS_BUZZER) && (RAKOS_HAS_BUZZER)

#include <rakos/lcd5_expander.h>
#include <Preferences.h>
#include <esp_io_expander.hpp>

static Preferences g_buzzer_prefs;
static uint8_t g_buzzer_vol_pct = 75;

bool AudioService::begin(uint32_t sample_rate) {
    (void)sample_rate;
    g_buzzer_prefs.begin("rakos_ui", false);
    volume_pct_ = g_buzzer_prefs.getUChar("audio_vol_pct", 75);
    if (volume_pct_ > 100) {
        volume_pct_ = 75;
    }
    g_buzzer_vol_pct = volume_pct_;

    auto *base = rakos::lcd5_expander();
    if (!base) {
        ready_ = false;
        return false;
    }
    if (!rakos::lcd5_expander_lock(200)) {
        ready_ = false;
        return false;
    }
    auto *exp = static_cast<esp_expander::CH422G *>(base);
    exp->enableOC_PushPull();
    exp->digitalWrite(EXPIO_BUZZER_DO, LOW);
    rakos::lcd5_expander_unlock();
    ready_ = true;
    Serial.println("[BSP] LCD-5 DO beep ready (isolated output)");
    return true;
}

void AudioService::setVolume(uint8_t volume_percent) {
    if (volume_percent > 100) {
        volume_percent = 100;
    }
    volume_pct_ = volume_percent;
    g_buzzer_vol_pct = volume_percent;
    g_buzzer_prefs.putUChar("audio_vol_pct", volume_pct_);
}

size_t AudioService::write(const uint8_t *data, size_t len) {
    (void)data;
    return len;
}

size_t AudioService::read(uint8_t *data, size_t len) {
    (void)data;
    return len;
}

void AudioService::beep(uint16_t freq_hz, uint16_t duration_ms) {
    (void)freq_hz;
    if (!ready_ || g_buzzer_vol_pct == 0 || duration_ms == 0) {
        return;
    }
    if (duration_ms > 80) {
        duration_ms = 80;
    }

    auto *exp = rakos::lcd5_expander();
    if (!exp || !rakos::lcd5_expander_lock(200)) {
        return;
    }
    exp->digitalWrite(EXPIO_BUZZER_DO, HIGH);
    rakos::lcd5_expander_unlock();
    delay(duration_ms);
    if (rakos::lcd5_expander_lock(200)) {
        exp->digitalWrite(EXPIO_BUZZER_DO, LOW);
        rakos::lcd5_expander_unlock();
    }
}

#else  /* no audio, no buzzer */

bool AudioService::begin(uint32_t sample_rate) {
    (void)sample_rate;
    ready_ = false;
    return false;
}

void AudioService::setVolume(uint8_t volume_percent) {
    (void)volume_percent;
}

size_t AudioService::write(const uint8_t *data, size_t len) {
    (void)data;
    return len;
}

size_t AudioService::read(uint8_t *data, size_t len) {
    (void)data;
    return len;
}

void AudioService::beep(uint16_t freq_hz, uint16_t duration_ms) {
    (void)freq_hz;
    (void)duration_ms;
}

#endif /* RAKOS_HAS_BUZZER */
#endif /* RAKOS_HAS_AUDIO */

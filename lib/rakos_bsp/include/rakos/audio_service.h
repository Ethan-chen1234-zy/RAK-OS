#pragma once

#include <Arduino.h>
#include <rakos/pin_config.h>

class AudioService {
public:
    bool begin(uint32_t sample_rate = AUDIO_DEFAULT_RATE);
    bool ready() const { return ready_; }

    void setVolume(uint8_t volume_percent);
    uint8_t volumePercent() const { return volume_pct_; }
    /** Short UI feedback tone via ES8311 speaker */
    void beep(uint16_t freq_hz = 1040, uint16_t duration_ms = 42);
    size_t write(const uint8_t *data, size_t len);
    size_t read(uint8_t *data, size_t len);

    uint32_t sampleRate() const { return sample_rate_; }

private:
    bool ready_ = false;
    uint32_t sample_rate_ = AUDIO_DEFAULT_RATE;
    uint8_t volume_pct_ = 75;
};

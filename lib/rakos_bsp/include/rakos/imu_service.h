#pragma once

#include <Arduino.h>

struct ImuSample {
    float ax = 0;
    float ay = 0;
    float az = 0;
    float gx = 0;
    float gy = 0;
    float gz = 0;
    bool accel_valid = false;
    bool gyro_valid = false;
};

class ImuService {
public:
    bool begin();
    bool ready() const { return ready_; }

    bool update(ImuSample &sample);

private:
    bool ready_ = false;
};

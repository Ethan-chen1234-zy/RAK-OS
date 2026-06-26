#include <rakos/power_service.h>
#include <rakos/pin_config.h>

#if RAKOS_HAS_AXP_PMU

#include <rakos/i2c_bus.h>
#include <Wire.h>
#include <XPowersLib.h>

static XPowersPMU g_pmu;

static int pmuReadReg(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, uint8_t len) {
    rakos::I2cLockGuard lock(200);
    if (!lock.locked()) {
        return -1;
    }

    Wire.beginTransmission(dev_addr);
    Wire.write(reg_addr);
    if (Wire.endTransmission(false) != 0) {
        return -1;
    }

    const uint8_t got = Wire.requestFrom(dev_addr, len);
    if (got != len) {
        return -1;
    }

    for (uint8_t i = 0; i < len; ++i) {
        data[i] = Wire.read();
    }
    return 0;
}

static int pmuWriteReg(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, uint8_t len) {
    rakos::I2cLockGuard lock(200);
    if (!lock.locked()) {
        return -1;
    }

    Wire.beginTransmission(dev_addr);
    Wire.write(reg_addr);
    Wire.write(data, len);
    return Wire.endTransmission() == 0 ? 0 : -1;
}

bool PowerService::begin() {
    if (!g_pmu.begin(AXP2101_I2C_ADDR, pmuReadReg, pmuWriteReg)) {
        Serial.println("[BSP] AXP2101 init failed");
        ready_ = false;
        status_.ready = false;
        return false;
    }

    g_pmu.disableTSPinMeasure();
    g_pmu.enableVbusVoltageMeasure();
    g_pmu.enableBattVoltageMeasure();
    g_pmu.enableSystemVoltageMeasure();
    g_pmu.enableBattDetection();

    g_pmu.setPrechargeCurr(XPOWERS_AXP2101_PRECHARGE_50MA);
    g_pmu.setChargerConstantCurr(XPOWERS_AXP2101_CHG_CUR_400MA);
    g_pmu.setChargerTerminationCurr(XPOWERS_AXP2101_CHG_ITERM_25MA);
    g_pmu.setChargeTargetVoltage(XPOWERS_AXP2101_CHG_VOL_4V2);

    g_pmu.setPowerKeyPressOffTime(XPOWERS_POWEROFF_4S);
    g_pmu.setPowerKeyPressOnTime(XPOWERS_POWERON_128MS);

    ready_ = true;
    status_.ready = true;
    last_read_ms_ = 0;

    Serial.printf("[BSP] AXP2101 ready\n");
    return true;
}

bool PowerService::rebegin() {
    ready_ = false;
    status_ = {};
    last_read_ms_ = 0;
    return begin();
}

void PowerService::update() {
    // Periodic PMIC polling disabled: XPowersLib reads crash after QMI/SD
    // init on the shared Wire bus. Shutdown via power_->shutdown() still works.
    (void)last_read_ms_;
}

void PowerService::shutdown() {
    if (!ready_) {
        return;
    }

    Serial.println("[BSP] AXP2101 shutdown");
    g_pmu.shutdown();
}

#else  /* !RAKOS_HAS_AXP_PMU */

bool PowerService::begin() {
    ready_ = false;
    status_.ready = false;
    Serial.println("[BSP] PMIC not present on this board");
    return false;
}

bool PowerService::rebegin() {
    return begin();
}

void PowerService::update() {}

void PowerService::shutdown() {
    Serial.println("[BSP] Shutdown not available (no PMIC)");
}

#endif /* RAKOS_HAS_AXP_PMU */

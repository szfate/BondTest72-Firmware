#include "adc.h"
#include <Arduino.h>
#include "../debug/log.h"

// Pin assignments — from docs/RP2350 PINMAP.md
static constexpr uint8_t ADC_PINS[3]  = {26, 27, 28};  // COM_D, COM_A, COM_C
static constexpr uint8_t DCDC_PSM_PIN = 23;             // HIGH = FPWM (less noise), LOW = PFM (power save)
static constexpr float   ADC_SCALE    = 3.3f / 4095.0f; // 12-bit, 3.3 V rail

void AdcDriver::begin() {
    analogReadResolution(12);
    for (uint8_t i = 0; i < 3; i++)
        pinMode(ADC_PINS[i], INPUT);
    pinMode(DCDC_PSM_PIN, OUTPUT);
    digitalWrite(DCDC_PSM_PIN, LOW);  // PFM by default

    AdcReadings r = readAll();
    LOG_I("adc init: COM_D=%.3fV COM_A=%.3fV COM_C=%.3fV",
          r.sense, r.prevNeighbour, r.nextNeighbour);
}

float AdcDriver::readVoltage(uint8_t channel) {
    if (channel >= 3) return 0.0f;
    return static_cast<float>(analogRead(ADC_PINS[channel])) * ADC_SCALE;
}

AdcReadings AdcDriver::readAll() {
    digitalWrite(DCDC_PSM_PIN, HIGH);
    AdcReadings r = { readVoltage(0), readVoltage(1), readVoltage(2) };
    digitalWrite(DCDC_PSM_PIN, LOW);
    return r;
}

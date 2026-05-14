#include "adc.h"
#include <Arduino.h>

// Pin assignments — from docs/RP2350 PINMAP.md
static constexpr uint8_t ADC_PINS[3] = {26, 27, 28};  // COM_D, COM_A, COM_C
static constexpr float   ADC_SCALE   = 3.3f / 4095.0f; // 12-bit, 3.3 V rail

void AdcDriver::begin() {
    analogReadResolution(12);
    for (uint8_t i = 0; i < 3; i++)
        pinMode(ADC_PINS[i], INPUT);
}

float AdcDriver::readVoltage(uint8_t channel) {
    if (channel >= 3) return 0.0f;
    return static_cast<float>(analogRead(ADC_PINS[channel])) * ADC_SCALE;
}

AdcReadings AdcDriver::readAll() {
    return { readVoltage(0), readVoltage(1), readVoltage(2) };  // sense, left, right
}

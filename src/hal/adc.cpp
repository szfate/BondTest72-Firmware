#include "adc.h"
#include <Arduino.h>
#include "debug/log.h"

// Pin assignments — from docs/RP2350 PINMAP.md
static constexpr uint8_t ADC_PINS[3]  = {26, 27, 28};  // COM_D, COM_A, COM_C
static constexpr uint8_t DCDC_PSM_PIN = 23;             // HIGH = FPWM (less noise), LOW = PFM (power save)
static constexpr float   ADC_SCALE    = VCC / 4095.0f; // 12-bit against the VCC rail

void AdcDriver::begin() {
    analogReadResolution(12);
    for (uint8_t i = 0; i < ADC_CHANNEL_COUNT; i++)
        pinMode(ADC_PINS[i], INPUT);
    pinMode(DCDC_PSM_PIN, OUTPUT);
    digitalWrite(DCDC_PSM_PIN, HIGH);  // FPWM always on — lower ADC noise, no current concern

    AdcReadings r = readAll();
    LOG_I("adc init: COM_D=%.3fV COM_A=%.3fV COM_C=%.3fV",
          r.sense, r.prevNeighbour, r.nextNeighbour);
}

float AdcDriver::readVoltage(AdcChannel channel) {
    uint8_t idx = static_cast<uint8_t>(channel);
    if (idx >= ADC_CHANNEL_COUNT) { LOG_E("adc: invalid channel %u", idx); return NAN; }  // never 0.0f — a real open reads 0V; NaN classifies as OPEN (every comparison against it is false)
    // discard first sample
    //(void)analogRead(ADC_PINS[idx]);
    return static_cast<float>(analogRead(ADC_PINS[idx])) * ADC_SCALE;
}

AdcReadings AdcDriver::readAll() {
    return { readVoltage(AdcChannel::ComD), readVoltage(AdcChannel::KelvinSense),
             readVoltage(AdcChannel::ComC) };
}

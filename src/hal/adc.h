#pragma once
#include <stdint.h>

static constexpr uint8_t ADC_CHANNEL_COUNT = 3;

struct AdcReadings {
    float sense;         // ADC0/GP26 — bond under test, injection+sense (27 kΩ pullup)
    float prevNeighbour; // ADC1/GP27 — previous neighbour short detection (1 MΩ divider)
    float nextNeighbour; // ADC2/GP28 — next neighbour short detection (1 MΩ divider)
};

class AdcDriver {
public:
    void        begin();
    float       readVoltage(uint8_t channel);  // 0=COM_D, 1=COM_A, 2=COM_C
    AdcReadings readAll();
};

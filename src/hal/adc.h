#pragma once
#include <stdint.h>

struct AdcReadings {
    float sense;         // ADC0/GP26 — bond under test, injection+sense (27 kΩ pullup)
    float leftNeighbour; // ADC1/GP27 — left neighbour short detection (1 MΩ divider)
    float rightNeighbour;// ADC2/GP28 — right neighbour short detection (1 MΩ divider)
};

class AdcDriver {
public:
    void        begin();
    float       readVoltage(uint8_t channel);  // 0=COM_D, 1=COM_A, 2=COM_C
    AdcReadings readAll();
};

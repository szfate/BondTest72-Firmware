#pragma once
#include <stdint.h>

static constexpr uint8_t ADC_CHANNEL_COUNT = 3;

// NOTE: only channel 1 (COM_A, the Kelvin sense line) is used by the
// production bond-test path. Channels 0/2 (COM_D/COM_C) are no longer wired
// into the sense path — they're left here for debug tooling only.
struct AdcReadings {
    float sense;         // ADC0/GP26 — COM_D, unused by test path (debug only)
    float prevNeighbour; // ADC1/GP27 — COM_A, Kelvin sense (the one that matters)
    float nextNeighbour; // ADC2/GP28 — COM_C, unused by test path (debug only)
};

class AdcDriver {
public:
    void        begin();
    float       readVoltage(uint8_t channel);  // 0=COM_D, 1=COM_A (Kelvin sense), 2=COM_C
    AdcReadings readAll();
};

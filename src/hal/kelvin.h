#pragma once
#include <stdint.h>
#include "mux.h"
#include "adc.h"

// Pullup drive strengths on the tester board (one per COM_C/D/E bus).
constexpr uint8_t PULLUP_LEVEL_COUNT = 3;

// Samples per CAP_SENSE charging curve.
constexpr uint8_t CAP_SENSE_SAMPLE_COUNT = 5;

// nominalUA is the design current through a good bond (reported to the host),
// not VCC/ohms — that's the short-circuit current, ~20% higher.
struct PullupLevel { Bus bus; float ohms; float nominalUA; };

// Lowest current first, so bypass caps never see strong current before weak.
extern const PullupLevel PULLUP_LEVELS[PULLUP_LEVEL_COUNT];

// Sample times (µs since drive start) measureKelvinCurve uses for a window.
// Exposed so the host protocol reports the schedule actually used.
void curveSampleTimesUs(uint16_t totalSettleUs, uint16_t* out /* [CAP_SENSE_SAMPLE_COUNT] */);

struct PadReading {
    float voltageV;       // COM_A Kelvin voltage
    float resistanceOhms; // apparent bond resistance
    bool  conducted;      // resistanceOhms < the caller's maxResistanceOhms
};

// One settled reading: forceCh driven through pullupBus and Kelvin-sensed,
// sinkCh grounded. Pass maxResistanceOhms <= 0 to ignore `conducted`.
PadReading measureKelvin(MuxController& mux, AdcDriver& adc,
                          uint8_t forceCh, uint8_t sinkCh, Bus pullupBus, float pullupOhms,
                          uint16_t settleUs, float maxResistanceOhms);

// True if any pullup level reads below thresholdV. For DUT presence and
// orientation checks.
bool kelvinAnyLevelBelow(MuxController& mux, AdcDriver& adc,
                          uint8_t forceCh, uint8_t sinkCh, float thresholdV, uint16_t settleUs);

// Like measureKelvin, but samples one continuous charge at the
// curveSampleTimesUs schedule. Only the last sample is settled enough to
// classify on; the earlier ones show the curve shape.
void measureKelvinCurve(MuxController& mux, AdcDriver& adc,
                         uint8_t forceCh, uint8_t sinkCh, Bus pullupBus, float pullupOhms,
                         uint16_t totalSettleUs, float maxResistanceOhms,
                         PadReading* out /* [CAP_SENSE_SAMPLE_COUNT] */);

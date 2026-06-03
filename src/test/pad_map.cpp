#include "pad_map.h"

void buildRingCases(TestCase* out, const uint8_t* adapterPins, const uint8_t* diePads, uint8_t count, uint8_t gnd,
                    const TestThresholds* thresholds, TestStrategy strategy, uint16_t settleMs) {
    for (uint8_t i = 0; i < count; i++) {
        out[i] = {
            .adapterPin = adapterPins[i],
            .gndPin     = gnd,
            .prevPin    = adapterPins[(i + count - 1) % count],  // prev — wraps around
            .nextPin    = adapterPins[(i + 1)         % count],  // next — wraps around
            .diePad     = diePads[i],
            .strategy   = strategy,
            .padType    = PadType::IO,
            .groupId    = 0,
            .settleMs   = settleMs,
            .thresholds = thresholds,
        };
    }
}

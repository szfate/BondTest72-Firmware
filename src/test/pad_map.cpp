#include "pad_map.h"

void buildRingCases(TestCase* out, const uint8_t* ring, const uint8_t* diePads, uint8_t count, uint8_t gnd) {
    for (uint8_t i = 0; i < count; i++) {
        out[i] = {
            ring[i],
            gnd,
            ring[(i + count - 1) % count],  // left  — wraps around
            ring[(i + 1)         % count],  // right — wraps around
            diePads[i],
        };
    }
}

#pragma once
#include <stdint.h>

struct MuxEntry {
    uint8_t chip;     // 0=U2, 1=U3, 2=U4
    uint8_t channel;  // X0–X23
};

extern const MuxEntry MUX_MAP[72];

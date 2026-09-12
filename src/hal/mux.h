#pragma once
#include <stdint.h>

static constexpr uint8_t MUX_CHANNEL_COUNT = 72;

// Y port → COM line mapping (matches PCB routing):
//   Y0 = COM_A (ADC1, GP27) — Kelvin sense; never drives current, only ever
//                             paired onto the same channel as whichever of
//                             COM_C/D/E is currently driving that pad
//   Y1 = COM_B             — tester GND (return path)
//   Y2 = COM_C             — 280 kΩ pullup (current injection)
//   Y3 = COM_D             — 27.4 kΩ pullup (current injection)
//   Y4 = COM_E             — 2.49 kΩ pullup (current injection)
//
// The enumerators are Y-port indices fed straight into the CH446X command
// encoding (see mux_map / computeAddr) — they are not validated against the
// PCB routing; that correspondence is by construction, not checked.
enum class Bus : uint8_t { A = 0, B = 1, C = 2, D = 3, E = 4 };

class MuxController {
public:
    void begin();
    void setChannel(uint8_t ch, Bus bus);
    void clearChannel(uint8_t ch, Bus bus);
    void clearAll();
};

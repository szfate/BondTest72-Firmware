#pragma once
#include <stdint.h>

static constexpr uint8_t MUX_CHANNEL_COUNT = 72;

// Y port → COM line mapping (matches PCB routing):
//   Y0 = COM_A (ADC1, GP27) — Kelvin sense; never drives current, only ever
//                             paired onto the same channel as whichever of
//                             COM_C/D/E is currently driving that pad
//   Y1 = COM_B             — tester GND (return path)
//   Y2 = COM_C             — 330 kΩ pullup (current injection)
//   Y3 = COM_D             — 33 kΩ pullup (current injection)
//   Y4 = COM_E             — 3.3 kΩ pullup (current injection)
enum class Bus : uint8_t { A = 0, B = 1, C = 2, D = 3, E = 4 };

class MuxController {
public:
    void begin();
    void setChannel(uint8_t ch, Bus bus);
    void clearChannel(uint8_t ch, Bus bus);
    void clearAll();
};

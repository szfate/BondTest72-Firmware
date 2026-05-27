#pragma once
#include <stdint.h>

static constexpr uint8_t MUX_CHANNEL_COUNT = 72;

// Y port → COM line mapping (matches PCB routing):
//   Y0 = COM_A (ADC1, GP27) — left neighbour sense
//   Y1 = COM_B             — tester GND (return path)
//   Y2 = COM_C (ADC2, GP28) — right neighbour sense
//   Y3 = COM_D (ADC0, GP26) — injection + sense
//   Y4 = COM_E             — spare
enum class Bus : uint8_t { A = 0, B = 1, C = 2, D = 3, E = 4 };

class MuxController {
public:
    void begin();
    void setChannel(uint8_t ch, Bus bus);
    void clearChannel(uint8_t ch, Bus bus);
    void clearAll();
};

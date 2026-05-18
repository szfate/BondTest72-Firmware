#pragma once
#include <stdint.h>

static constexpr uint8_t NO_NEIGHBOUR = 0xFF;

// One measurement: which pad to test, which pad is the GND reference,
// and which physical neighbours to sense for shorts.
// All indices are mezzanine connector pins (adapter-space).
//
// Use NO_NEIGHBOUR when a physical neighbour is a GND pad. GND pads are the
// current injection point (BUS_D), so routing one to BUS_A or BUS_C would read
// ~0 V and falsely flag as a short to neighbour.
struct TestCase {
    uint8_t mezPin;    // mez pin under test (BUS_B return path)
    uint8_t gndPin;    // mez GND pin — injection/reference (BUS_D sense)
    uint8_t leftPin;   // mez pin of left neighbour (BUS_A), NO_NEIGHBOUR if GND pad
    uint8_t rightPin;  // mez pin of right neighbour (BUS_C), NO_NEIGHBOUR if GND pad
};

struct PadMap {
    uint8_t          id;
    const char*      name;
    const TestCase*  cases;
    uint8_t          caseCount;
    uint8_t          presencePadA;        // mezzanine GND pin → Bus::D (27K pull-up, sense)
    uint8_t          presencePadB;        // mezzanine GND pin → Bus::B (GND return)
    float            presenceThresholdV;  // COM_D below this → DUT GND completing circuit → DUT present

    float            senseGoodMin;        // COM_D: below → SHORT_GND   (e.g. 0.5 V)
    float            senseGoodMax;        // COM_D: above → OPEN         (e.g. 0.7 V)
    float            neighbourGoodMin;    // COM_A/C: below → short       (e.g. 0.3 V)
    float            neighbourGoodMax;    // COM_A/C: above → out of range (e.g. 2.0 V)
};

// Helper for simple ring-layout dies: populates out[count] from an ordered
// ring of pads sharing one GND. Neighbours wrap around at ring ends.
// Caller owns the output array (typically a static local in pad_map_registry.cpp).
void buildRingCases(TestCase* out, const uint8_t* ring, uint8_t count, uint8_t gnd);

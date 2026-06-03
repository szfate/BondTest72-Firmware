#pragma once
#include <stdint.h>

static constexpr uint8_t NO_NEIGHBOUR = 0xFF;

struct TestThresholds {
    float senseGoodMin;      // COM_D: below → SHORT_GND
    float senseGoodMax;      // COM_D: above → OPEN
    float neighbourGoodMin;  // COM_A/C: below → short
    float neighbourGoodMax;  // COM_A/C: above → out of range
};

enum class TestStrategy : uint8_t {
    STANDARD,    // sense + neighbours
    SKIP_SENSE,  // capacitive pad: neighbours only, bond sense not checked
    DISCHARGE,   // nop: short adapterPin+gndPin to Bus::B for settleMs; discharges cap, no result
    PRECHARGE,   // nop: adapterPin→Bus::D, gndPin→Bus::B for settleMs; charges cap, no result
    CAP_SENSE,   // cap charging test: 3 readings spaced settleMs apart (adapterPin→Bus::D, gndPin→Bus::B);
                 // pass if final reading ≥ senseGoodMin — confirms bond conducted to charge the cap
};

enum class PadType : uint8_t {
    IO,
    VDDIO,
    VDD_CORE,
    PWR_AUX,
    GND,
};

// One measurement step: which pad to test, which is GND, which are neighbours,
// how to measure it, and what values are acceptable.
// All pin indices are adapter pins (1-indexed).
//
// Use NO_NEIGHBOUR when a physical neighbour is a GND pad. GND pads are the
// current injection point (BUS_D), so routing one to BUS_A or BUS_C would read
// ~0 V and falsely flag as a short.
//
// DISCHARGE steps use gndPin for both BUS_B and BUS_D to short the cap to GND;
// no result is recorded. thresholds may be nullptr for DISCHARGE.
struct TestCase {
    uint8_t               adapterPin;  // adapter pin under test (BUS_B return path)
    uint8_t               gndPin;      // adapter GND pin — injection/reference (BUS_D sense)
    uint8_t               prevPin;     // adapter pin of previous neighbour (BUS_A, die N-1), NO_NEIGHBOUR if GND pad
    uint8_t               nextPin;     // adapter pin of next neighbour (BUS_C, die N+1), NO_NEIGHBOUR if GND pad
    uint8_t               diePad;      // die pad number (for logging; 0 for DISCHARGE steps)
    TestStrategy          strategy;
    PadType               padType;
    uint8_t               groupId;     // 0 = must pass individually; >0 = belongs to a PadGroup
    uint16_t              settleMs;
    const TestThresholds* thresholds;  // nullptr valid only for DISCHARGE
};

// A group of pads that collectively need minPass members to pass.
struct PadGroup {
    uint8_t     id;
    uint8_t     minPass;
    const char* name;
};

struct PadMap {
    uint8_t          id;
    const char*      name;
    const TestCase*  cases;
    uint8_t          caseCount;
    uint8_t          presencePadA;        // adapter GND pin → Bus::D (27K pull-up, sense)
    uint8_t          presencePadB;        // adapter GND pin → Bus::B (GND return)
    float            presenceThresholdV;  // COM_D below this → DUT present
    const PadGroup*  padGroups;
    uint8_t          padGroupCount;
};

// Helper for simple ring-layout dies: populates out[count] from an ordered
// ring of pads sharing one GND. Neighbours wrap around at ring ends.
// Caller owns the output array (typically a static local in pad_map_registry.cpp).
void buildRingCases(TestCase* out, const uint8_t* adapterPins, const uint8_t* diePads, uint8_t count, uint8_t gnd,
                    const TestThresholds* thresholds, TestStrategy strategy = TestStrategy::STANDARD, uint16_t settleMs = 2);

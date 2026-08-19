#pragma once
#include <stdint.h>
#include "pad_map.h"

enum class BondResult : uint8_t {
    GOOD,   // at least one reading showed activity
    OPEN,   // every reading sat at ~VCC — no conduction detected at all
};

constexpr uint8_t READING_COUNT = 2 * PULLUP_LEVEL_COUNT;  // all pullup levels x {forward,reverse}

// Fixed slot layout: [0 .. PULLUP_LEVEL_COUNT-1] = forward, [PULLUP_LEVEL_COUNT ..
// 2*PULLUP_LEVEL_COUNT-1] = reverse. What each slot MEANS depends on which
// TestStrategy produced it (see pad_map.h and host_protocol.cpp's `method=`
// wire field, which tells a reader which interpretation applies):
//   STANDARD  — one slot per pullup level, highest resistance/lowest current first.
//   CAP_SENSE — one slot per time-sample of a single continuous 3.3k charging
//               event (see measureKelvinCurve), earliest to latest — NOT a
//               current level. Classification uses only the last (most-
//               settled) slot in each direction; earlier slots are for
//               curve-shape visibility.
struct PadReading {
    float voltageV;      // COM_A Kelvin voltage
    float resistanceOhms; // apparent bond resistance, informational only
    bool  conducted;      // voltageV sufficiently below VCC ⇒ activity detected
};

struct PadResult {
    BondResult  bond;
    PadReading  readings[READING_COUNT];
};

constexpr uint8_t MAX_DUT_SLOTS = 5;

struct SlotResult {
    PadResult byChannel[72];   // indexed by tester channel (= adapter_pin − 1); only tested entries are valid
    uint8_t   testedCount;
    uint8_t   goodCount;
    bool      present;          // DUT detected in this slot
    bool      tested;           // test completed for this slot
};

enum class TestOutcome : uint8_t {
    PASS,
    FAIL,
    FAIL_DUT_REMOVED,
    WRONG_ORIENTATION,
};

struct TestResult {
    SlotResult  slots[MAX_DUT_SLOTS];
    uint8_t     slotCount;
    TestOutcome outcome;
};

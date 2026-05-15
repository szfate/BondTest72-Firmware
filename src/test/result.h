#pragma once
#include <stdint.h>

enum class BondResult : uint8_t {
    GOOD,
    OPEN,
    SHORT_GND,
};

struct PadResult {
    BondResult bond;
    bool       leftShort;
    bool       rightShort;
};

struct SlotResult {
    PadResult pads[72];   // indexed by mez channel; only IO pad entries are valid
    uint8_t   testedCount;
    uint8_t   goodCount;
};

enum class TestOutcome : uint8_t {
    PASS,
    FAIL,
    FAIL_DUT_REMOVED,
    WRONG_ORIENTATION,
};

struct TestResult {
    SlotResult  slots[5];   // [0] only for Mezzanine70; all 5 for Mezzanine70x5
    uint8_t     slotCount;
    TestOutcome outcome;
};

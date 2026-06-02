#pragma once
#include <stdint.h>

enum class BondResult : uint8_t {
    GOOD,
    OPEN,
    SHORT_GND,
};

struct PadResult {
    BondResult bond;
    bool       prevShort;
    bool       nextShort;
    float      senseV;   // COM_D sense voltage
    float      prevV;    // COM_A prev-neighbour voltage (0 if not tested)
    float      nextV;    // COM_C next-neighbour voltage (0 if not tested)
};

constexpr uint8_t MAX_DUT_SLOTS = 5;

struct SlotResult {
    PadResult byChannel[72];   // indexed by tester channel (= mez_pin − 1); only tested entries are valid
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

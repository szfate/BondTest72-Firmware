#pragma once
#include <stdint.h>
#include "pad_map.h"
#include "hal/kelvin.h"

enum class BondResult : uint8_t {
    NOT_TESTED,  // zero-init default for untested channels — never emitted on the wire
    GOOD,   // at least one reading showed activity
    OPEN,   // every reading sat at ~VCC — no conduction detected at all
};

constexpr uint8_t READINGS_PER_DIR = PULLUP_LEVEL_COUNT > CAP_SENSE_SAMPLE_COUNT
    ? PULLUP_LEVEL_COUNT : CAP_SENSE_SAMPLE_COUNT;

// How many of a direction group's slots are used. STANDARD: one per pullup
// level, in PULLUP_LEVELS order. CAP_SENSE: one per curve sample, earliest first.
constexpr uint8_t readingsPerDir(TestStrategy s) {
    return s == TestStrategy::CAP_SENSE ? CAP_SENSE_SAMPLE_COUNT : PULLUP_LEVEL_COUNT;
}

struct PadResult {
    BondResult  bond;
    PadReading  fwd[READINGS_PER_DIR];  // zeroed when the direction isn't measured
    PadReading  rev[READINGS_PER_DIR];
};

// Each slot costs ~9 KB; no multi-slot adapter exists yet.
constexpr uint8_t MAX_DUT_SLOTS = 1;

struct SlotResult {
    PadResult byChannel[72];   // by tester channel; only tested entries are valid
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

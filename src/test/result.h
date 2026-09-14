#pragma once
#include <stdint.h>
#include "pad_map.h"
#include "hal/kelvin.h"

enum class BondResult : uint8_t {
    NOT_TESTED,  // zero-init default for untested channels — never emitted on the wire
    GOOD,   // at least one reading showed activity
    OPEN,   // every reading sat at ~VCC — no conduction detected at all
};

// LATCHUP INVESTIGATION: per-pad-map since the MOSB bring-up — the sweep
// direction lives in PadMap::directions (pad_map.h). REVERSE_ONLY remains the
// setting for cap-equipped DUTs: forward is the only sweep step that charges
// the adapter-side bypass cap to ~VCC (forceCh = adapterCh, hard-grounded
// afterwards in drainAndRelease) — the charged-node sequence implicated in
// the CH446X latch-up (see hal/kelvin.cpp); in reverse, adapterCh sits on
// Bus::B for the whole measurement, so that cap never charges. BOTH /
// FORWARD_ONLY are safe only for capless DUTs (MOSB). Whichever direction
// group isn't measured stays zeroed.

// Each direction group is sized for the strategy with the most per-direction
// samples, even though STANDARD and CAP_SENSE use different per-direction
// counts (see below).
constexpr uint8_t READINGS_PER_DIR = PULLUP_LEVEL_COUNT > CAP_SENSE_SAMPLE_COUNT
    ? PULLUP_LEVEL_COUNT : CAP_SENSE_SAMPLE_COUNT;

// One direction group per PadResult.fwd / PadResult.rev, each indexed from 0.
// How many of a group's slots are meaningful, and what each slot MEANS,
// depends on which TestStrategy produced it (see pad_map.h and
// host_protocol.cpp's `method=` wire field, which tells a reader which
// interpretation applies):
//   STANDARD  — one slot per pullup level, highest resistance/lowest current
//               first; trailing slots (PULLUP_LEVEL_COUNT..READINGS_PER_DIR-1)
//               unused.
//   CAP_SENSE — one slot per time-sample of a single continuous 2.49k charging
//               event (see measureKelvinCurve), earliest to latest — NOT a
//               current level. Classification uses only the last (most-settled)
//               slot in each direction; earlier slots are for curve-shape
//               visibility.

// Single source of the group-length rule documented above: how many readings
// each direction group holds for a strategy.
constexpr uint8_t readingsPerDir(TestStrategy s) {
    return s == TestStrategy::CAP_SENSE ? CAP_SENSE_SAMPLE_COUNT : PULLUP_LEVEL_COUNT;
}

struct PadResult {
    BondResult  bond;
    PadReading  fwd[READINGS_PER_DIR];  // forward measurements (zeroed unless forward is measured — see PadMap::directions)
    PadReading  rev[READINGS_PER_DIR];  // reverse measurements
};

// DUT slot headroom. Kept as a knob so a future multi-slot adapter only needs
// to bump this and return the right getDutCount() — no struct/protocol changes.
// No multi-slot adapter has been manufactured; 1 is correct for the foreseeable
// future. (Was 5 — cost ~44 KB of static/stack for unused slot arrays.)
constexpr uint8_t MAX_DUT_SLOTS = 1;

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

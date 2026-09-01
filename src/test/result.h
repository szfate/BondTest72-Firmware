#pragma once
#include <stdint.h>
#include "pad_map.h"

enum class BondResult : uint8_t {
    GOOD,   // at least one reading showed activity
    OPEN,   // every reading sat at ~VCC — no conduction detected at all
};

// Which sweep direction(s) to measure. Selects the method= field on PAD
// lines (STD/CAP = both, STD_FW/CAP_FW = forward only, STD_REV/CAP_REV =
// reverse only) so the host knows which direction group(s) to expect — see
// sendPadResult in host_protocol.cpp.
enum class MeasureDirections : uint8_t {
    BOTH,
    REVERSE_ONLY,
    FORWARD_ONLY,
};

constexpr bool measuresForward(MeasureDirections d) { return d != MeasureDirections::REVERSE_ONLY; }
constexpr bool measuresReverse(MeasureDirections d) { return d != MeasureDirections::FORWARD_ONLY; }

// LATCHUP INVESTIGATION: REVERSE_ONLY is the current build. Forward is the
// only sweep step that charges the adapter-side bypass cap to ~VCC (forceCh =
// adapterCh, hard-grounded afterwards in drainAndRelease) — the charged-node
// sequence implicated in the CH446X latch-up (see hal/kelvin.cpp); in
// reverse, adapterCh sits on Bus::B for the whole measurement, so that cap
// never charges. FORWARD_ONLY is kept available for root-cause isolation.
// Whichever half of PadResult.readings a direction doesn't measure stays
// zeroed.
constexpr MeasureDirections MEASURE_DIRECTIONS = MeasureDirections::REVERSE_ONLY;

// Sized for the strategy with the most per-direction samples, so the array
// is shared even though STANDARD and CAP_SENSE use different per-direction
// counts (see below).
constexpr uint8_t READINGS_PER_DIR = PULLUP_LEVEL_COUNT > CAP_SENSE_SAMPLE_COUNT
    ? PULLUP_LEVEL_COUNT : CAP_SENSE_SAMPLE_COUNT;
constexpr uint8_t READING_COUNT = 2 * READINGS_PER_DIR;

// Fixed slot layout: [0 .. N-1] = forward (all-zero unless forward is
// measured — see MEASURE_DIRECTIONS above), [N .. 2N-1] = reverse, where N is
// the strategy's own per-direction count — NOT always READINGS_PER_DIR, so
// unused trailing slots may exist when a strategy's N is smaller than the
// array's. What each slot MEANS, and what N is, depends on which
// TestStrategy produced it (see pad_map.h and host_protocol.cpp's `method=`
// wire field, which tells a reader which interpretation applies):
//   STANDARD  — N = PULLUP_LEVEL_COUNT, one slot per pullup level, highest
//               resistance/lowest current first.
//   CAP_SENSE — N = CAP_SENSE_SAMPLE_COUNT, one slot per time-sample of a
//               single continuous 3.3k charging event (see
//               measureKelvinCurve), earliest to latest — NOT a current
//               level. Classification uses only the last (most-settled)
//               slot in each direction; earlier slots are for curve-shape
//               visibility.
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

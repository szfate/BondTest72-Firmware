#pragma once
#include <stdint.h>

// Number of pullup drive strengths swept per pad (330k/33k/3.3k today).
// Single source of truth — result.h derives READING_COUNT from this, and
// test_runner.cpp's PULLUP_LEVELS array must have exactly this many entries.
// Bumping this requires: more entries in PULLUP_LEVELS (test_runner.cpp),
// and — since only COM_C/D/E are wired as pullup buses today — additional
// hardware pullup networks to actually drive the extra currents.
constexpr uint8_t PULLUP_LEVEL_COUNT = 3;

// Number of voltage samples measureKelvinCurve takes across a single
// CAP_SENSE charging event (see hal/kelvin.cpp's curveSampleTimesUs for the
// actual time schedule). Independent of PULLUP_LEVEL_COUNT — CAP_SENSE
// samples one continuous charge at a fixed pullup, not different pullups —
// so this can be tuned purely for curve-shape resolution.
constexpr uint8_t CAP_SENSE_SAMPLE_COUNT = 5;

// A reading counts as "activity" if its apparent bond resistance (computed
// from the Kelvin voltage and the pullup value — see measureKelvin) is
// below this ceiling. We don't try to judge bond quality — dies vary too
// much for a single fixed criterion — we just ask "did anything happen at
// all?". No activity across all 6 readings ⇒ OPEN.
//
// A resistance ceiling (not a voltage margin) is deliberate: a fixed
// voltage margin applied at every pullup level makes the weakest pullup
// (330k) far more sensitive to stray leakage/coupling than the strongest
// (3.3k) — a multi-MΩ parasitic path can dip a 330k reading by more than a
// small voltage margin while never being anywhere near a real bond's
// resistance. A resistance ceiling normalizes across pullup levels instead.
// Confirmed real bonds (incl. weak ESD-diode-mediated ones) conduct at the
// 33k/3.3k levels up to ~9.2kΩ/~1.6kΩ (the 330k level alone can read up to
// ~81kΩ for a real bond, but that's fine since only one of the 6 readings
// needs to pass). Confirmed-unbonded pads have never dropped below ~256kΩ
// at any of the 6 readings — see kThresh in pad_map_registry.cpp for the
// current value and full rationale.
struct TestThresholds {
    float maxBondResistanceOhms;
};

enum class TestStrategy : uint8_t {
    STANDARD,    // Kelvin resistance sweep: {330k,33k,3.3k} per MEASURE_DIRECTIONS (see result.h)
    DISCHARGE,   // nop: short adapterPin+gndPin to Bus::B for settleUs; discharges cap, no result
    CAP_SENSE,   // For pads with a real bypass/decoupling cap (VDDIO/VDD_CORE/PWR_AUX) where
                 // STANDARD's 330k/33k levels can never settle in practical time (τ = R·C —
                 // at 1µF, 330k gives τ≈330ms, 33k gives τ≈33ms, impractical per-pad). Uses
                 // only the 3.3k level, one direction per MEASURE_DIRECTIONS (result.h) — in
                 // the reverse-only sweep the DUT bypass cap is on the grounded sink side and
                 // the charged node is the die-side net (τ is DUT-dependent, ~2.3ms on the
                 // 1x1 die). `settleUs` here is the post-discharge settle time (want ≥~3τ of
                 // THAT net — OPEN must charge past the resistance-ceiling voltage to
                 // classify; GOOD errs safe — not the short STANDARD default) — see
                 // measureKelvin's built-in discharge step for how the node gets to a known
                 // baseline before each reading.
};

enum class PadType : uint8_t {
    IO,
    VDDIO,
    VDD_CORE,
    PWR_AUX,
    GND,
};

// One measurement step: which pad to test, which is GND/reference, how to
// measure it, and what counts as conducting. All pin indices are adapter
// pins (1-indexed).
//
// DISCHARGE steps short both adapterPin and gndPin to Bus::B to drain the cap;
// no result is recorded. thresholds may be nullptr for DISCHARGE.
struct TestCase {
    uint8_t               adapterPin;  // adapter pin under test
    uint8_t               gndPin;      // adapter GND pin — reference/return for the sweep
    uint8_t               diePad;      // die pad number (for logging; 0 for DISCHARGE steps)
    TestStrategy          strategy;
    PadType               padType;
    uint16_t              settleUs;
    const TestThresholds* thresholds;  // nullptr valid only for DISCHARGE
};

struct PadMap {
    uint8_t          id;
    const char*      name;
    const TestCase*  cases;
    uint8_t          caseCount;
    uint8_t          presencePadA;        // adapter GND pin → Bus::D (33K pull-up) + Bus::A (Kelvin sense)
    uint8_t          presencePadB;        // adapter GND pin → Bus::B (GND return)
    float            presenceThresholdV;  // COM_A below this → DUT present
};

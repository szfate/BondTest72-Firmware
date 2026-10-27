#pragma once
#include <stdint.h>
#include "adapter/adapter_base.h"  // AdapterPinout

// A reading counts as activity if its apparent bond resistance is below the
// ceiling; a pad with no activity in any reading is OPEN. We don't judge bond
// quality, only "did anything conduct". A resistance ceiling rather than a
// voltage margin, because a fixed voltage margin makes the 280k level far more
// sensitive to stray leakage than the 2.49k level. Value: kThresh in
// pad_map_registry.cpp.
struct TestThresholds {
    float maxBondResistanceOhms;
};

// Also selects the method= suffix on PAD lines (none / _FW / _REV).
enum class MeasureDirections : uint8_t {
    BOTH,
    REVERSE_ONLY,
    FORWARD_ONLY,
};

constexpr bool measuresForward(MeasureDirections d) { return d != MeasureDirections::REVERSE_ONLY; }
constexpr bool measuresReverse(MeasureDirections d) { return d != MeasureDirections::FORWARD_ONLY; }

enum class TestStrategy : uint8_t {
    STANDARD,    // one reading per pullup level
    DISCHARGE,   // ground both pins for settleUs; no result
    CAP_SENSE,   // pads with a bypass cap, where the 280k/27.4k levels would take
                 // tens to hundreds of ms to settle: 2.49k only, sampled as a
                 // charging curve. settleUs must cover ≥~3τ of the die-side net
                 // so an OPEN pad charges past the ceiling.
};

enum class PadType : uint8_t {
    IO,
    VDDIO,
    VDD_CORE,
    PWR_AUX,
    GND,
};

// One measurement step. Pin numbers are adapter pins (1-indexed).
struct TestCase {
    uint8_t               adapterPin;  // pin under test
    uint8_t               gndPin;      // reference/return
    uint8_t               diePad;      // for reporting; 0 for DISCHARGE
    TestStrategy          strategy;
    PadType               padType;
    uint16_t              settleUs;
    const TestThresholds* thresholds;  // nullptr only for DISCHARGE
};

struct PadMap {
    uint8_t          id;
    const char*      name;                // sent to the host by LIST_PADMAPS
    AdapterPinout    pinout;              // any adapter with this pinout can run the map
    const TestCase*  cases;
    uint8_t          caseCount;
    uint8_t          presencePadA;        // driven + sensed for DUT presence
    uint8_t          presencePadB;        // grounded for DUT presence
    float            presenceThresholdV;  // sense below this → DUT present
    bool             checkOrientation;    // only when a flipped DUT reliably reads open
    MeasureDirections directions;         // REVERSE_ONLY when the DUT has bypass caps: forward
                                          // drive charges the adapter-side cap, the latch-up
                                          // trigger (hal/kelvin.cpp). BOTH only for capless DUTs.
};

#include "pad_map_registry.h"

// — Pad map 1: Mezzanine70 ——————————————————————————————————————————————————
// 56 IO pads in physical die-pad ring order (die pads 1–70, skipping GND/PWR).
// Source: docs/MEZ_CONNECTOR_MAP.md
// GND mez pins (all shorted on adapter): 10, 18, 25, 45, 53, 61 — any one works.
// Gaps in the ring (where a GND/PWR pad sits between two IO pads) are noted inline.

static TestCase _pm1Cases[56];

static void buildPm1() {
    static const uint8_t ring[] = {
        // die  1– 8  (mez descending 8→1)
         8,  7,  6,  5,  4,  3,  2,  1,
        // [gap: die GND 9, VDDIO 74, VDD_CORE 72, GND 73 — mez 10,9,11]
        // die 10–17  (mez descending 70→63)
        70, 69, 68, 67, 66, 65, 64, 63,
        // [gap: VDDIO die 18, GND die 19 — mez 62,61]
        // die 20–25  (mez descending 60→55)
        60, 59, 58, 57, 56, 55,
        // [gap: PWR_AUX die 26, GND die 27 — mez 54,53]
        // die 28–33  (mez descending 52→47)
        52, 51, 50, 49, 48, 47,
        // [gap: GND die 34, VDD_CORE die 35, GND die 36, VDDIO die 37 — mez –,46,45,44]
        // die 38–45  (mez descending 43→36)
        43, 42, 41, 40, 39, 38, 37, 36,
        // [gap: GND die 46 — no individual mez]
        // die 47–56  (mez descending 35→26)
        35, 34, 33, 32, 31, 30, 29, 28, 27, 26,
        // [gap: GND die 57, VDDIO die 58 — mez 25,24]
        // die 59–62  (mez descending 23→20)
        23, 22, 21, 20,
        // [gap: GND die 63, PWR_AUX die 64 — mez 18,19]
        // die 65–70  (mez descending 17→12)
        17, 16, 15, 14, 13, 12,
        // [ring wraps: GND die 71, VDD_CORE die 72, GND die 73, VDDIO die 74 — mez –,11,10,9]
    };
    static const uint8_t gnd = 10;  // mez pin 10 = die GND 73 (all GND pins equivalent)
    buildRingCases(_pm1Cases, ring, sizeof(ring), gnd);
}

static const PadMap _maps[] = {
    {
        .id                 = 1,
        .name               = "Mezzanine70",
        .cases              = _pm1Cases,
        .caseCount          = 56,
        .presencePadA       = 10,   // mez GND pin (die GND 73)
        .presencePadB       = 53,   // mez GND pin (die GND 27) — opposite side of die
        .presenceThresholdV = 0.3f,
        .senseGoodMin       = 0.5f,
        .senseGoodMax       = 0.7f,
        .neighbourGoodMin   = 0.3f,
        .neighbourGoodMax   = 2.0f,
    },
};

static constexpr uint8_t MAP_COUNT = sizeof(_maps) / sizeof(_maps[0]);

// Called once from setup() before any pad map is used.
static bool _built = false;
static void ensureBuilt() {
    if (_built) return;
    buildPm1();
    _built = true;
}

// ——————————————————————————————————————————————————————————————————————————

const PadMap* PadMapRegistry::find(uint8_t id) {
    ensureBuilt();
    for (uint8_t i = 0; i < MAP_COUNT; i++) {
        if (_maps[i].id == id) return &_maps[i];
    }
    return nullptr;
}

const PadMap* PadMapRegistry::all() {
    ensureBuilt();
    return _maps;
}

uint8_t PadMapRegistry::count() {
    return MAP_COUNT;
}

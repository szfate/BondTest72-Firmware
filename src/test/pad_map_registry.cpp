#include "pad_map_registry.h"

// Shared threshold instances — reference by pointer from TestCase.
// Define one per logical pad category; multiple pads can share the same instance.
static constexpr TestThresholds kThreshIoPm1  = { 0.2f, 0.8f, 0.2f, 2.5f };  // pm1 IO bonds
static constexpr TestThresholds kThreshPwrPm1 = { 0.2f, 0.8f, 0.2f, 2.5f };  // pm1 VDDIO/VDD_CORE (sense checked)
static constexpr TestThresholds kThreshIoPm2  = { 0.2f, 0.8f, 0.4f, 2.0f };  // pm2 IO bonds
static constexpr TestThresholds kThreshPwrPm2 = { 0.2f, 0.8f, 0.4f, 2.0f };  // pm2 VDDIO/VDD_CORE (sense checked)
// PWR_AUX has a cap to GND — sense unreliable; SKIP_SENSE strategy, neighbours still checked.
static constexpr TestThresholds kThreshPwrAuxPm1 = { 0.0f, 0.0f, 0.2f, 2.5f };  // sense fields unused
static constexpr TestThresholds kThreshPwrAuxPm2 = { 0.0f, 0.0f, 0.4f, 2.0f };  // sense fields unused

// — Pad map 1: Mezzanine70 ——————————————————————————————————————————————————
// 55 IO pads + 8 VDD/PWR pads = 63 cases total.
// Source: docs/MEZ_CONNECTOR_MAP.md — all mez numbers are ADAPTER-side.
// GND mez pins (all shorted on adapter): 10, 18, 26, 46, 53, 61 — any one works.
// Gaps in the ring (where a GND/PWR pad sits between two IO pads) are noted inline.

static TestCase _pm1Cases[65];  // 55 IO + 8 VDD/PWR + 2 DISCHARGE (mez34/die44 omitted — unconnected PCB trace on v1 boards)

static void buildPm1() {
    static const uint8_t ring[] = {
        // die  1– 8  (adapter mez ascending 63→70)
        63, 64, 65, 66, 67, 68, 69, 70,
        // [gap: GND die 9 — no individual mez]
        // die 10–17  (adapter mez ascending 1→8)
         1,  2,  3,  4,  5,  6,  7,  8,
        // [gap: VDDIO die 18 mez 9, GND die 19 mez 10]
        // die 20–25  (adapter mez ascending 11→16)
        11, 12, 13, 14, 15, 16,
        // [gap: PWR_AUX die 26 mez 17, GND die 27 mez 18]
        // die 28–33  (adapter mez ascending 19→24)
        19, 20, 21, 22, 23, 24,
        // [gap: GND die 34 no mez, VDD_CORE die 35 mez 25, GND die 36 mez 26, VDDIO die 37 mez 27]
        // die 38–43, 45  (mez 28→33, 35 — mez34/die44 skipped: unconnected trace on v1 PCB)
        28, 29, 30, 31, 32, 33, 35,
        // [gap: GND die 46 — no individual mez]
        // die 47–56  (adapter mez ascending 36→45)
        36, 37, 38, 39, 40, 41, 42, 43, 44, 45,
        // [gap: GND die 57 mez 46, VDDIO die 58 mez 47]
        // die 59–62  (adapter mez ascending 48→51)
        48, 49, 50, 51,
        // [gap: PWR_AUX die 64 mez 52, GND die 63 mez 53]
        // die 65–70  (adapter mez ascending 54→59)
        54, 55, 56, 57, 58, 59,
        // [ring wraps: GND die 71 no mez, VDD_CORE die 72 mez 60, GND die 73 mez 61, VDDIO die 74 mez 62]
    };
    static const uint8_t diePads[] = {
         1,  2,  3,  4,  5,  6,  7,  8,   // die  1– 8
        10, 11, 12, 13, 14, 15, 16, 17,   // die 10–17
        20, 21, 22, 23, 24, 25,            // die 20–25
        28, 29, 30, 31, 32, 33,            // die 28–33
        38, 39, 40, 41, 42, 43, 45,        // die 38–43, 45 (die44 skipped)
        47, 48, 49, 50, 51, 52, 53, 54, 55, 56,  // die 47–56
        59, 60, 61, 62,                    // die 59–62
        65, 66, 67, 68, 69, 70,            // die 65–70
    };
    static_assert(sizeof(ring) == sizeof(diePads), "ring/diePads length mismatch");
    static const uint8_t gnd = 10;  // mez pin 10 = die GND 19 (all GND pins equivalent)
    buildRingCases(_pm1Cases, ring, diePads, sizeof(ring), gnd, &kThreshIoPm1);

    _pm1Cases[15].nextPin = NO_NEIGHBOUR;  // mez8  → mez11  (mez10 GND in gap)
    _pm1Cases[16].prevPin = NO_NEIGHBOUR;  // mez11 ← mez8
    _pm1Cases[21].nextPin = NO_NEIGHBOUR;  // mez16 → mez19  (mez18 GND in gap)
    _pm1Cases[22].prevPin = NO_NEIGHBOUR;  // mez19 ← mez16
    _pm1Cases[27].nextPin = NO_NEIGHBOUR;  // mez24 → mez28  (mez26 GND in gap)
    _pm1Cases[28].prevPin = NO_NEIGHBOUR;  // mez28 ← mez24
    _pm1Cases[44].nextPin = NO_NEIGHBOUR;  // mez45 → mez48  (mez46 GND in gap)
    _pm1Cases[45].prevPin = NO_NEIGHBOUR;  // mez48 ← mez45
    _pm1Cases[48].nextPin = NO_NEIGHBOUR;  // mez51 → mez54  (mez53 GND in gap)
    _pm1Cases[49].prevPin = NO_NEIGHBOUR;  // mez54 ← mez51
    _pm1Cases[54].nextPin = NO_NEIGHBOUR;  // mez59 → mez63  (mez61 GND in gap, ring wrap)
    _pm1Cases[ 0].prevPin = NO_NEIGHBOUR;  // mez63 ← mez59

    // VDD/PWR pads — 8 extra cases appended after the ring.
    // Neighbors suppressed on GND-adjacent sides; IO neighbors retained where possible.
    _pm1Cases[55] = { .mezPin =  9, .gndPin = gnd, .prevPin =  8, .nextPin = NO_NEIGHBOUR, .diePad = 18, .strategy = TestStrategy::STANDARD,   .padType = PadType::VDDIO,   .settleMs = 2,   .thresholds = &kThreshPwrPm1    };  // die18 VDDIO   — prev=mez8(die17),  next=mez10 GND
    _pm1Cases[56] = { .mezPin = 17, .gndPin = gnd, .prevPin = NO_NEIGHBOUR, .nextPin = NO_NEIGHBOUR, .diePad =  0, .strategy = TestStrategy::DISCHARGE, .padType = PadType::PWR_AUX, .settleMs = 100, .thresholds = nullptr         };  // discharge die26 PWR_AUX cap before measurement
    _pm1Cases[57] = { .mezPin = 17, .gndPin = gnd, .prevPin = 16, .nextPin = NO_NEIGHBOUR, .diePad = 26, .strategy = TestStrategy::SKIP_SENSE, .padType = PadType::PWR_AUX, .settleMs = 0,   .thresholds = &kThreshPwrAuxPm1 };  // die26 PWR_AUX — capacitive, settle=0 after discharge
    _pm1Cases[58] = { .mezPin = 25, .gndPin = gnd, .prevPin = NO_NEIGHBOUR, .nextPin = NO_NEIGHBOUR, .diePad = 35, .strategy = TestStrategy::STANDARD,   .padType = PadType::VDD_CORE,.settleMs = 2,   .thresholds = &kThreshPwrPm1    };  // die35 VDD_CORE — both GND neighbors
    _pm1Cases[59] = { .mezPin = 27, .gndPin = gnd, .prevPin = NO_NEIGHBOUR, .nextPin = 28, .diePad = 37, .strategy = TestStrategy::STANDARD,   .padType = PadType::VDDIO,   .settleMs = 2,   .thresholds = &kThreshPwrPm1    };  // die37 VDDIO   — prev=mez26 GND,    next=mez28(die38)
    _pm1Cases[60] = { .mezPin = 47, .gndPin = gnd, .prevPin = NO_NEIGHBOUR, .nextPin = 48, .diePad = 58, .strategy = TestStrategy::STANDARD,   .padType = PadType::VDDIO,   .settleMs = 2,   .thresholds = &kThreshPwrPm1    };  // die58 VDDIO   — prev=mez46 GND,    next=mez48(die59)
    _pm1Cases[61] = { .mezPin = 52, .gndPin = gnd, .prevPin = NO_NEIGHBOUR, .nextPin = NO_NEIGHBOUR, .diePad =  0, .strategy = TestStrategy::DISCHARGE, .padType = PadType::PWR_AUX, .settleMs = 100, .thresholds = nullptr         };  // discharge die64 PWR_AUX cap before measurement
    _pm1Cases[62] = { .mezPin = 52, .gndPin = gnd, .prevPin = NO_NEIGHBOUR, .nextPin = 54, .diePad = 64, .strategy = TestStrategy::STANDARD,   .padType = PadType::PWR_AUX, .settleMs = 0,   .thresholds = &kThreshPwrPm1    };  // die64 PWR_AUX — settle=0 after discharge
    _pm1Cases[63] = { .mezPin = 60, .gndPin = gnd, .prevPin = NO_NEIGHBOUR, .nextPin = NO_NEIGHBOUR, .diePad = 72, .strategy = TestStrategy::STANDARD,   .padType = PadType::VDD_CORE,.settleMs = 2,   .thresholds = &kThreshPwrPm1    };  // die72 VDD_CORE — both GND neighbors
    _pm1Cases[64] = { .mezPin = 62, .gndPin = gnd, .prevPin = NO_NEIGHBOUR, .nextPin = 63, .diePad = 74, .strategy = TestStrategy::STANDARD,   .padType = PadType::VDDIO,   .settleMs = 2,   .thresholds = &kThreshPwrPm1    };  // die74 VDDIO   — prev=mez61 GND,    next=mez63(die1)
}

// — Pad map 2: Mezzanine70 v2 ——————————————————————————————————————————————————

static TestCase _pm2Cases[66];

static void buildPm2() {
    static const uint8_t ring[] = {
        // die  1– 8  (adapter mez ascending 63→70)
        63, 64, 65, 66, 67, 68, 69, 70,
        // [gap: GND die 9 — no individual mez]
        // die 10–17  (adapter mez ascending 1→8)
         1,  2,  3,  4,  5,  6,  7,  8,
        // [gap: VDDIO die 18 mez 9, GND die 19 mez 10]
        // die 20–25  (adapter mez ascending 11→16)
        11, 12, 13, 14, 15, 16,
        // [gap: PWR_AUX die 26 mez 17, GND die 27 mez 18]
        // die 28–33  (adapter mez ascending 19→24)
        19, 20, 21, 22, 23, 24,
        // [gap: GND die 34 no mez, VDD_CORE die 35 mez 25, GND die 36 mez 26, VDDIO die 37 mez 27]
        // die 38–45  (adapter mez ascending 28→35)
        28, 29, 30, 31, 32, 33, 34, 35,
        // [gap: GND die 46 — no individual mez]
        // die 47–56  (adapter mez ascending 36→45)
        36, 37, 38, 39, 40, 41, 42, 43, 44, 45,
        // [gap: GND die 57 mez 46, VDDIO die 58 mez 47]
        // die 59–62  (adapter mez ascending 48→51)
        48, 49, 50, 51,
        // [gap: PWR_AUX die 64 mez 52, GND die 63 mez 53]
        // die 65–70  (adapter mez ascending 54→59)
        54, 55, 56, 57, 58, 59,
        // [ring wraps: GND die 71 no mez, VDD_CORE die 72 mez 60, GND die 73 mez 61, VDDIO die 74 mez 62]
    };
    static const uint8_t diePads[] = {
         1,  2,  3,  4,  5,  6,  7,  8,   // die  1– 8
        10, 11, 12, 13, 14, 15, 16, 17,   // die 10–17
        20, 21, 22, 23, 24, 25,            // die 20–25
        28, 29, 30, 31, 32, 33,            // die 28–33
        38, 39, 40, 41, 42, 43, 44, 45,   // die 38–45 (die44 included in v2)
        47, 48, 49, 50, 51, 52, 53, 54, 55, 56,  // die 47–56
        59, 60, 61, 62,                    // die 59–62
        65, 66, 67, 68, 69, 70,            // die 65–70
    };
    static_assert(sizeof(ring) == sizeof(diePads), "ring/diePads length mismatch");
    static const uint8_t gnd = 10;  // mez pin 10 = die GND 19 (all GND pins equivalent)
    buildRingCases(_pm2Cases, ring, diePads, sizeof(ring), gnd, &kThreshIoPm2);

    _pm2Cases[15].nextPin = NO_NEIGHBOUR;  // mez8  → mez11  (mez10 GND in gap)
    _pm2Cases[16].prevPin = NO_NEIGHBOUR;  // mez11 ← mez8
    _pm2Cases[21].nextPin = NO_NEIGHBOUR;  // mez16 → mez19  (mez18 GND in gap)
    _pm2Cases[22].prevPin = NO_NEIGHBOUR;  // mez19 ← mez16
    _pm2Cases[27].nextPin = NO_NEIGHBOUR;  // mez24 → mez28  (mez26 GND in gap)
    _pm2Cases[28].prevPin = NO_NEIGHBOUR;  // mez28 ← mez24
    _pm2Cases[45].nextPin = NO_NEIGHBOUR;  // mez45 → mez48  (mez46 GND in gap)
    _pm2Cases[46].prevPin = NO_NEIGHBOUR;  // mez48 ← mez45
    _pm2Cases[49].nextPin = NO_NEIGHBOUR;  // mez51 → mez54  (mez53 GND in gap)
    _pm2Cases[50].prevPin = NO_NEIGHBOUR;  // mez54 ← mez51
    _pm2Cases[55].nextPin = NO_NEIGHBOUR;  // mez59 → mez63  (mez61 GND in gap, ring wrap)
    _pm2Cases[ 0].prevPin = NO_NEIGHBOUR;  // mez63 ← mez59

    _pm2Cases[56] = { .mezPin =  9, .gndPin = gnd, .prevPin =  8, .nextPin = NO_NEIGHBOUR, .diePad = 18, .strategy = TestStrategy::STANDARD,   .padType = PadType::VDDIO,   .settleMs = 2,   .thresholds = &kThreshPwrPm2    };  // die18 VDDIO   — prev=mez8(die17),  next=mez10 GND
    _pm2Cases[57] = { .mezPin = 17, .gndPin = gnd, .prevPin = NO_NEIGHBOUR, .nextPin = NO_NEIGHBOUR, .diePad =  0, .strategy = TestStrategy::DISCHARGE, .padType = PadType::PWR_AUX, .settleMs = 100, .thresholds = nullptr         };  // discharge die26 PWR_AUX cap before measurement
    _pm2Cases[58] = { .mezPin = 17, .gndPin = gnd, .prevPin = 16, .nextPin = NO_NEIGHBOUR, .diePad = 26, .strategy = TestStrategy::SKIP_SENSE, .padType = PadType::PWR_AUX, .settleMs = 0,   .thresholds = &kThreshPwrAuxPm2 };  // die26 PWR_AUX — capacitive, settle=0 after discharge
    _pm2Cases[59] = { .mezPin = 25, .gndPin = gnd, .prevPin = NO_NEIGHBOUR, .nextPin = NO_NEIGHBOUR, .diePad = 35, .strategy = TestStrategy::STANDARD,   .padType = PadType::VDD_CORE,.settleMs = 2,   .thresholds = &kThreshPwrPm2    };  // die35 VDD_CORE — both GND neighbors
    _pm2Cases[60] = { .mezPin = 27, .gndPin = gnd, .prevPin = NO_NEIGHBOUR, .nextPin = 28, .diePad = 37, .strategy = TestStrategy::STANDARD,   .padType = PadType::VDDIO,   .settleMs = 2,   .thresholds = &kThreshPwrPm2    };  // die37 VDDIO   — prev=mez26 GND,    next=mez28(die38)
    _pm2Cases[61] = { .mezPin = 47, .gndPin = gnd, .prevPin = NO_NEIGHBOUR, .nextPin = 48, .diePad = 58, .strategy = TestStrategy::STANDARD,   .padType = PadType::VDDIO,   .settleMs = 2,   .thresholds = &kThreshPwrPm2    };  // die58 VDDIO   — prev=mez46 GND,    next=mez48(die59)
    _pm2Cases[62] = { .mezPin = 52, .gndPin = gnd, .prevPin = NO_NEIGHBOUR, .nextPin = NO_NEIGHBOUR, .diePad =  0, .strategy = TestStrategy::DISCHARGE, .padType = PadType::PWR_AUX, .settleMs = 100, .thresholds = nullptr         };  // discharge die64 PWR_AUX cap before measurement
    _pm2Cases[63] = { .mezPin = 52, .gndPin = gnd, .prevPin = NO_NEIGHBOUR, .nextPin = 54, .diePad = 64, .strategy = TestStrategy::STANDARD,   .padType = PadType::PWR_AUX, .settleMs = 0,   .thresholds = &kThreshPwrPm2    };  // die64 PWR_AUX — settle=0 after discharge
    _pm2Cases[64] = { .mezPin = 60, .gndPin = gnd, .prevPin = NO_NEIGHBOUR, .nextPin = NO_NEIGHBOUR, .diePad = 72, .strategy = TestStrategy::STANDARD,   .padType = PadType::VDD_CORE,.settleMs = 2,   .thresholds = &kThreshPwrPm2    };  // die72 VDD_CORE — both GND neighbors
    _pm2Cases[65] = { .mezPin = 62, .gndPin = gnd, .prevPin = NO_NEIGHBOUR, .nextPin = 63, .diePad = 74, .strategy = TestStrategy::STANDARD,   .padType = PadType::VDDIO,   .settleMs = 2,   .thresholds = &kThreshPwrPm2    };  // die74 VDDIO   — prev=mez61 GND,    next=mez63(die1)
}

static const PadMap _maps[] = {
    {
        .id                 = 1,
        .name               = "Mezzanine70 v1",
        .cases              = _pm1Cases,
        .caseCount          = 65,
        .presencePadA       = 10,   // mez GND pin (die GND 19)
        .presencePadB       = 53,   // mez GND pin (die GND 63) — opposite side of die
        .presenceThresholdV = 0.3f,
    },
    {
        .id                 = 2,
        .name               = "Mezzanine70 v2",
        .cases              = _pm2Cases,
        .caseCount          = 66,
        .presencePadA       = 10,
        .presencePadB       = 53,
        .presenceThresholdV = 0.3f,
    },
};

static constexpr uint8_t MAP_COUNT = sizeof(_maps) / sizeof(_maps[0]);

static bool _built = false;
static void ensureBuilt() {
    if (_built) return;
    buildPm1();
    buildPm2();
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

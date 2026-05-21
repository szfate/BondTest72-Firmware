#include "pad_map_registry.h"

static constexpr TestThresholds kThreshIoPm1     = { 0.2f, 0.8f, 0.2f, 1.5f };
static constexpr TestThresholds kThreshPwrPm1    = { 0.2f, 0.8f, 0.2f, 1.5f };
static constexpr TestThresholds kThreshIoPm2     = { 0.2f, 0.8f, 0.2f, 1.5f };
static constexpr TestThresholds kThreshPwrPm2    = { 0.2f, 0.8f, 0.2f, 1.5f };
// PWR_AUX has a cap to GND — sense and neighbour voltages are cap-state-dependent; accept 0.2–3.0 V throughout.
static constexpr TestThresholds kThreshPwrAuxPm1 = { 0.2f, 3.0f, 0.2f, 3.0f };
static constexpr TestThresholds kThreshPwrAuxPm2 = { 0.2f, 3.0f, 0.2f, 3.0f };

#define NON NO_NEIGHBOUR
static constexpr uint8_t GND = 10;  // mez10 = die GND 19; all GND pins equivalent

// IO case shorthand — args: mez, prevMez, nextMez, diePad
#define IO1(m_, p_, n_, d_)  \
    { .mezPin=(m_), .gndPin=GND, .prevPin=(p_), .nextPin=(n_), \
      .diePad=(d_), .strategy=TestStrategy::STANDARD, .padType=PadType::IO, \
      .groupId=0, .settleMs=1, .thresholds=&kThreshIoPm1 }
#define IO2(m_, p_, n_, d_)  \
    { .mezPin=(m_), .gndPin=GND, .prevPin=(p_), .nextPin=(n_), \
      .diePad=(d_), .strategy=TestStrategy::STANDARD, .padType=PadType::IO, \
      .groupId=0, .settleMs=1, .thresholds=&kThreshIoPm2 }

// — Pad map 1: Mezzanine70 v1 ——————————————————————————————————————————————
// 55 IO + 8 VDD/PWR = 63 cases. Source: docs/MEZ_CONNECTOR_MAP.md.
// GND mez pins (equivalent): 10, 18, 26, 46, 53, 61.
// mez34/die44 omitted — unconnected PCB trace on v1 boards.

static const TestCase _pm1Cases[] = {
    // ── die  1– 8  (mez 63–70) ─────────────────────────────────────────────
    IO1( 63, NON,  64,  1),
    IO1( 64,  63,  65,  2),
    IO1( 65,  64,  66,  3),
    IO1( 66,  65,  67,  4),
    IO1( 67,  66,  68,  5),
    IO1( 68,  67,  69,  6),
    IO1( 69,  68,  70,  7),
    IO1( 70,  69,   1,  8),  // [gap: GND die 9]
    // ── die 10–17  (mez  1– 8) ─────────────────────────────────────────────
    IO1(  1,  70,   2, 10),
    IO1(  2,   1,   3, 11),
    IO1(  3,   2,   4, 12),
    IO1(  4,   3,   5, 13),
    IO1(  5,   4,   6, 14),
    IO1(  6,   5,   7, 15),
    IO1(  7,   6,   8, 16),
    IO1(  8,   7, NON, 17),  // [gap: VDDIO mez9 die18, GND mez10 die19]
    // ── die 20–25  (mez 11–16) ─────────────────────────────────────────────
    IO1( 11, NON,  12, 20),
    IO1( 12,  11,  13, 21),
    IO1( 13,  12,  14, 22),
    IO1( 14,  13,  15, 23),
    IO1( 15,  14,  16, 24),
    IO1( 16,  15, NON, 25),  // [gap: PWR_AUX mez17 die26, GND mez18 die27]
    // ── die 28–33  (mez 19–24) ─────────────────────────────────────────────
    IO1( 19, NON,  20, 28),
    IO1( 20,  19,  21, 29),
    IO1( 21,  20,  22, 30),
    IO1( 22,  21,  23, 31),
    IO1( 23,  22,  24, 32),
    IO1( 24,  23, NON, 33),  // [gap: VDD_CORE mez25, VDDIO mez27]
    // ── die 38–43, 45  (mez 28–33, 35) — mez34/die44 unconnected on v1 ────
    IO1( 28, NON,  29, 38),
    IO1( 29,  28,  30, 39),
    IO1( 30,  29,  31, 40),
    IO1( 31,  30,  32, 41),
    IO1( 32,  31,  33, 42),
    IO1( 33,  32,  35, 43),
    IO1( 35,  33,  36, 45),  // die 44 / mez 34 skipped
    // ── die 47–56  (mez 36–45) ─────────────────────────────────────────────
    IO1( 36,  35,  37, 47),
    IO1( 37,  36,  38, 48),
    IO1( 38,  37,  39, 49),
    IO1( 39,  38,  40, 50),
    IO1( 40,  39,  41, 51),
    IO1( 41,  40,  42, 52),
    IO1( 42,  41,  43, 53),
    IO1( 43,  42,  44, 54),
    IO1( 44,  43,  45, 55),
    IO1( 45,  44, NON, 56),  // [gap: GND mez46 die57, VDDIO mez47 die58]
    // ── die 59–62  (mez 48–51) ─────────────────────────────────────────────
    IO1( 48, NON,  49, 59),
    IO1( 49,  48,  50, 60),
    IO1( 50,  49,  51, 61),
    IO1( 51,  50, NON, 62),  // [gap: PWR_AUX mez52 die64, GND mez53 die63]
    // ── die 65–70  (mez 54–59) ─────────────────────────────────────────────
    IO1( 54, NON,  55, 65),
    IO1( 55,  54,  56, 66),
    IO1( 56,  55,  57, 67),
    IO1( 57,  56,  58, 68),
    IO1( 58,  57,  59, 69),
    IO1( 59,  58, NON, 70),  // [gap: ring wraps through GND mez61]
    // ── VDD/PWR ────────────────────────────────────────────────────────────
    // groupId 1 = PWR_AUX (pass if ≥1), groupId 2 = VDDIO (pass if ≥1)
    { .mezPin =  9, .gndPin = GND, .prevPin =  8, .nextPin = NON, .diePad = 18, .strategy = TestStrategy::STANDARD, .padType = PadType::VDDIO,    .groupId = 2, .settleMs = 2, .thresholds = &kThreshPwrPm1    },  // die 18 VDDIO
    { .mezPin = 17, .gndPin = GND, .prevPin = 16, .nextPin = NON, .diePad = 26, .strategy = TestStrategy::STANDARD, .padType = PadType::PWR_AUX,  .groupId = 1, .settleMs = 0, .thresholds = &kThreshPwrAuxPm1 },  // die 26 PWR_AUX
    { .mezPin = 25, .gndPin = GND, .prevPin = NON, .nextPin = NON, .diePad = 35, .strategy = TestStrategy::STANDARD, .padType = PadType::VDD_CORE, .groupId = 0, .settleMs = 2, .thresholds = &kThreshPwrPm1    },  // die 35 VDD_CORE
    { .mezPin = 27, .gndPin = GND, .prevPin = NON, .nextPin =  28, .diePad = 37, .strategy = TestStrategy::STANDARD, .padType = PadType::VDDIO,   .groupId = 2, .settleMs = 2, .thresholds = &kThreshPwrPm1    },  // die 37 VDDIO
    { .mezPin = 47, .gndPin = GND, .prevPin = NON, .nextPin =  48, .diePad = 58, .strategy = TestStrategy::STANDARD, .padType = PadType::VDDIO,   .groupId = 2, .settleMs = 2, .thresholds = &kThreshPwrPm1    },  // die 58 VDDIO
    { .mezPin = 52, .gndPin = GND, .prevPin = NON, .nextPin =  54, .diePad = 64, .strategy = TestStrategy::STANDARD, .padType = PadType::PWR_AUX,  .groupId = 1, .settleMs = 0, .thresholds = &kThreshPwrAuxPm1 },  // die 64 PWR_AUX
    { .mezPin = 60, .gndPin = GND, .prevPin = NON, .nextPin = NON, .diePad = 72, .strategy = TestStrategy::STANDARD, .padType = PadType::VDD_CORE, .groupId = 0, .settleMs = 2, .thresholds = &kThreshPwrPm1    },  // die 72 VDD_CORE
    { .mezPin = 62, .gndPin = GND, .prevPin = NON, .nextPin =  63, .diePad = 74, .strategy = TestStrategy::STANDARD, .padType = PadType::VDDIO,   .groupId = 2, .settleMs = 2, .thresholds = &kThreshPwrPm1    },  // die 74 VDDIO
};
static_assert(sizeof(_pm1Cases) / sizeof(_pm1Cases[0]) == 63, "pm1 case count mismatch");

// — Pad map 2: Mezzanine70 v2 ——————————————————————————————————————————————
// 56 IO + 8 VDD/PWR = 64 cases. Identical to pm1 except mez34/die44 is connected.

static const TestCase _pm2Cases[] = {
    // ── die  1– 8  (mez 63–70) ─────────────────────────────────────────────
    IO2( 63, NON,  64,  1),
    IO2( 64,  63,  65,  2),
    IO2( 65,  64,  66,  3),
    IO2( 66,  65,  67,  4),
    IO2( 67,  66,  68,  5),
    IO2( 68,  67,  69,  6),
    IO2( 69,  68,  70,  7),
    IO2( 70,  69,   1,  8),  // [gap: GND die 9]
    // ── die 10–17  (mez  1– 8) ─────────────────────────────────────────────
    IO2(  1,  70,   2, 10),
    IO2(  2,   1,   3, 11),
    IO2(  3,   2,   4, 12),
    IO2(  4,   3,   5, 13),
    IO2(  5,   4,   6, 14),
    IO2(  6,   5,   7, 15),
    IO2(  7,   6,   8, 16),
    IO2(  8,   7, NON, 17),  // [gap: VDDIO mez9 die18, GND mez10 die19]
    // ── die 20–25  (mez 11–16) ─────────────────────────────────────────────
    IO2( 11, NON,  12, 20),
    IO2( 12,  11,  13, 21),
    IO2( 13,  12,  14, 22),
    IO2( 14,  13,  15, 23),
    IO2( 15,  14,  16, 24),
    IO2( 16,  15, NON, 25),  // [gap: PWR_AUX mez17 die26, GND mez18 die27]
    // ── die 28–33  (mez 19–24) ─────────────────────────────────────────────
    IO2( 19, NON,  20, 28),
    IO2( 20,  19,  21, 29),
    IO2( 21,  20,  22, 30),
    IO2( 22,  21,  23, 31),
    IO2( 23,  22,  24, 32),
    IO2( 24,  23, NON, 33),  // [gap: VDD_CORE mez25, VDDIO mez27]
    // ── die 38–45  (mez 28–35) — mez34/die44 connected in v2 ───────────────
    IO2( 28, NON,  29, 38),
    IO2( 29,  28,  30, 39),
    IO2( 30,  29,  31, 40),
    IO2( 31,  30,  32, 41),
    IO2( 32,  31,  33, 42),
    IO2( 33,  32,  34, 43),
    IO2( 34,  33,  35, 44),  // die 44 / mez 34 — connected in v2
    IO2( 35,  34,  36, 45),
    // ── die 47–56  (mez 36–45) ─────────────────────────────────────────────
    IO2( 36,  35,  37, 47),
    IO2( 37,  36,  38, 48),
    IO2( 38,  37,  39, 49),
    IO2( 39,  38,  40, 50),
    IO2( 40,  39,  41, 51),
    IO2( 41,  40,  42, 52),
    IO2( 42,  41,  43, 53),
    IO2( 43,  42,  44, 54),
    IO2( 44,  43,  45, 55),
    IO2( 45,  44, NON, 56),  // [gap: GND mez46 die57, VDDIO mez47 die58]
    // ── die 59–62  (mez 48–51) ─────────────────────────────────────────────
    IO2( 48, NON,  49, 59),
    IO2( 49,  48,  50, 60),
    IO2( 50,  49,  51, 61),
    IO2( 51,  50, NON, 62),  // [gap: PWR_AUX mez52 die64, GND mez53 die63]
    // ── die 65–70  (mez 54–59) ─────────────────────────────────────────────
    IO2( 54, NON,  55, 65),
    IO2( 55,  54,  56, 66),
    IO2( 56,  55,  57, 67),
    IO2( 57,  56,  58, 68),
    IO2( 58,  57,  59, 69),
    IO2( 59,  58, NON, 70),  // [gap: ring wraps through GND mez61]
    // ── VDD/PWR ────────────────────────────────────────────────────────────
    { .mezPin =  9, .gndPin = GND, .prevPin =  8, .nextPin = NON, .diePad = 18, .strategy = TestStrategy::STANDARD, .padType = PadType::VDDIO,    .groupId = 2, .settleMs = 2, .thresholds = &kThreshPwrPm2    },  // die 18 VDDIO
    { .mezPin = 17, .gndPin = GND, .prevPin = 16, .nextPin = NON, .diePad = 26, .strategy = TestStrategy::STANDARD, .padType = PadType::PWR_AUX,  .groupId = 1, .settleMs = 0, .thresholds = &kThreshPwrAuxPm2 },  // die 26 PWR_AUX
    { .mezPin = 25, .gndPin = GND, .prevPin = NON, .nextPin = NON, .diePad = 35, .strategy = TestStrategy::STANDARD, .padType = PadType::VDD_CORE, .groupId = 0, .settleMs = 2, .thresholds = &kThreshPwrPm2    },  // die 35 VDD_CORE
    { .mezPin = 27, .gndPin = GND, .prevPin = NON, .nextPin =  28, .diePad = 37, .strategy = TestStrategy::STANDARD, .padType = PadType::VDDIO,   .groupId = 2, .settleMs = 2, .thresholds = &kThreshPwrPm2    },  // die 37 VDDIO
    { .mezPin = 47, .gndPin = GND, .prevPin = NON, .nextPin =  48, .diePad = 58, .strategy = TestStrategy::STANDARD, .padType = PadType::VDDIO,   .groupId = 2, .settleMs = 2, .thresholds = &kThreshPwrPm2    },  // die 58 VDDIO
    { .mezPin = 52, .gndPin = GND, .prevPin = NON, .nextPin =  54, .diePad = 64, .strategy = TestStrategy::STANDARD, .padType = PadType::PWR_AUX,  .groupId = 1, .settleMs = 0, .thresholds = &kThreshPwrAuxPm2 },  // die 64 PWR_AUX
    { .mezPin = 60, .gndPin = GND, .prevPin = NON, .nextPin = NON, .diePad = 72, .strategy = TestStrategy::STANDARD, .padType = PadType::VDD_CORE, .groupId = 0, .settleMs = 2, .thresholds = &kThreshPwrPm2    },  // die 72 VDD_CORE
    { .mezPin = 62, .gndPin = GND, .prevPin = NON, .nextPin =  63, .diePad = 74, .strategy = TestStrategy::STANDARD, .padType = PadType::VDDIO,   .groupId = 2, .settleMs = 2, .thresholds = &kThreshPwrPm2    },  // die 74 VDDIO
};
static_assert(sizeof(_pm2Cases) / sizeof(_pm2Cases[0]) == 64, "pm2 case count mismatch");

#undef IO1
#undef IO2
#undef NON

// Groups shared across pad maps (same die, same grouping rules)
static const PadGroup _groups[] = {
    { .id = 1, .minPass = 1, .name = "PWR_AUX" },  // 2 pins: die26, die64 — pass if ≥1 bond
    { .id = 2, .minPass = 1, .name = "VDDIO"   },  // 4 pins: die18, die37, die58, die74 — pass if ≥1 bond
};
static constexpr uint8_t GROUP_COUNT = sizeof(_groups) / sizeof(_groups[0]);

static const PadMap _maps[] = {
    {
        .id                 = 1,
        .name               = "Mezzanine70 v1",
        .cases              = _pm1Cases,
        .caseCount          = 63,
        .presencePadA       = 10,
        .presencePadB       = 53,
        .presenceThresholdV = 0.3f,
        .padGroups          = _groups,
        .padGroupCount      = GROUP_COUNT,
    },
    {
        .id                 = 2,
        .name               = "Mezzanine70 v2",
        .cases              = _pm2Cases,
        .caseCount          = 64,
        .presencePadA       = 10,
        .presencePadB       = 53,
        .presenceThresholdV = 0.3f,
        .padGroups          = _groups,
        .padGroupCount      = GROUP_COUNT,
    },
};
static constexpr uint8_t MAP_COUNT = sizeof(_maps) / sizeof(_maps[0]);

// ——————————————————————————————————————————————————————————————————————————

const PadMap* PadMapRegistry::find(uint8_t id) {
    for (uint8_t i = 0; i < MAP_COUNT; i++) {
        if (_maps[i].id == id) return &_maps[i];
    }
    return nullptr;
}

const PadMap* PadMapRegistry::all() {
    return _maps;
}

uint8_t PadMapRegistry::count() {
    return MAP_COUNT;
}

#include "pad_map_registry.h"

static constexpr TestThresholds kThreshIO     = { 0.2f, 0.8f, 0.2f, 1.5f };
static constexpr TestThresholds kThreshPwr    = { 0.2f, 0.8f, 0.2f, 1.5f };
// PWR_AUX has a bypass cap to GND that distorts bond-sense readings regardless of direction.
// Use SKIP_SENSE — bond result is always GOOD; neighbour shorts are still checked.
static constexpr TestThresholds kThreshPwrAux = { 0.2f, 3.0f, 0.2f, 3.0f };

static constexpr TestThresholds kThreshIoPm1     = kThreshIO;
static constexpr TestThresholds kThreshPwrPm1    = kThreshPwr;
static constexpr TestThresholds kThreshPwrAuxPm1 = kThreshPwrAux;
static constexpr TestThresholds kThreshIoPm2     = kThreshIO;
static constexpr TestThresholds kThreshPwrPm2    = kThreshPwr;
static constexpr TestThresholds kThreshPwrAuxPm2 = kThreshPwrAux;
static constexpr TestThresholds kThreshIoPm3     = kThreshIO;
static constexpr TestThresholds kThreshPwrPm3    = kThreshPwr;
static constexpr TestThresholds kThreshPwrAuxPm3 = kThreshPwrAux;

#define NON NO_NEIGHBOUR
static constexpr uint8_t GND  = 10;  // adapter pin 10, die pad 18; all GND pins equivalent (1x1)
static constexpr uint8_t GND3 = 26;  // adapter pin 26, die pad 41; all GND pins equivalent (1x0p5)

// IO case shorthand — args: adapterPin, prevAdapterPin, nextAdapterPin, diePad
#define IO1(m_, p_, n_, d_)  \
    { .adapterPin=(m_), .gndPin=GND,  .prevPin=(p_), .nextPin=(n_), \
      .diePad=(d_), .strategy=TestStrategy::STANDARD, .padType=PadType::IO, \
      .groupId=0, .settleMs=1, .thresholds=&kThreshIoPm1 }
#define IO2(m_, p_, n_, d_)  \
    { .adapterPin=(m_), .gndPin=GND,  .prevPin=(p_), .nextPin=(n_), \
      .diePad=(d_), .strategy=TestStrategy::STANDARD, .padType=PadType::IO, \
      .groupId=0, .settleMs=1, .thresholds=&kThreshIoPm2 }
#define IO3(m_, p_, n_, d_)  \
    { .adapterPin=(m_), .gndPin=GND3, .prevPin=(p_), .nextPin=(n_), \
      .diePad=(d_), .strategy=TestStrategy::STANDARD, .padType=PadType::IO, \
      .groupId=0, .settleMs=1, .thresholds=&kThreshIoPm3 }

// — Pad map 1: Mezzanine70 v1 ——————————————————————————————————————————————
// 56 IO + 7 VDD/PWR = 63 cases. Source: docs/DUT_PADMAP_1X1.md.
// GND adapter pins (equivalent): 10, 18, 26, 46, 53, 61.
// adapter pin 52/die pad 63 (PWR_AUX) omitted — unconnected PCB trace on v1 boards.
// adapter pin 34/die pad 43 IS connected on v1 (contrary to earlier assumption).

static const TestCase _pm1Cases[] = {
    // ── die pads  0– 7 (apin 63–70) ─────────────────────────────────────────
    IO1( 63, NON,  64,  0),
    IO1( 64,  63,  65,  1),
    IO1( 65,  64,  66,  2),
    IO1( 66,  65,  67,  3),
    IO1( 67,  66,  68,  4),
    IO1( 68,  67,  69,  5),
    IO1( 69,  68,  70,  6),
    IO1( 70,  69,   1,  7),  // [gap: GND die pad 8]
    // ── die pads  9–16 (apin  1– 8) ─────────────────────────────────────────
    IO1(  1,  70,   2,  9),
    IO1(  2,   1,   3, 10),
    IO1(  3,   2,   4, 11),
    IO1(  4,   3,   5, 12),
    IO1(  5,   4,   6, 13),
    IO1(  6,   5,   7, 14),
    IO1(  7,   6,   8, 15),
    IO1(  8,   7, NON, 16),  // [gap: VDD IO apin9 die pad 17, GND apin10 die pad 18]
    // ── die pads 19–24 (apin 11–16) ─────────────────────────────────────────
    IO1( 11, NON,  12, 19),
    IO1( 12,  11,  13, 20),
    IO1( 13,  12,  14, 21),
    IO1( 14,  13,  15, 22),
    IO1( 15,  14,  16, 23),
    IO1( 16,  15, NON, 24),  // [gap: PWR Aux apin17 die pad 25, GND apin18 die pad 26]
    // ── die pads 27–32 (apin 19–24) ─────────────────────────────────────────
    IO1( 19, NON,  20, 27),
    IO1( 20,  19,  21, 28),
    IO1( 21,  20,  22, 29),
    IO1( 22,  21,  23, 30),
    IO1( 23,  22,  24, 31),
    IO1( 24,  23, NON, 32),  // [gap: VDD Core apin25 die pad 34, VDD IO apin27 die pad 36]
    // ── die pads 37–44 (apin 28–35) ─────────────────────────────────────────
    IO1( 28, NON,  29, 37),
    IO1( 29,  28,  30, 38),
    IO1( 30,  29,  31, 39),
    IO1( 31,  30,  32, 40),
    IO1( 32,  31,  33, 41),
    IO1( 33,  32,  34, 42),
    IO1( 34,  33,  35, 43),
    IO1( 35,  34,  36, 44),
    // ── die pads 46–55 (apin 36–45) ─────────────────────────────────────────
    IO1( 36,  35,  37, 46),
    IO1( 37,  36,  38, 47),
    IO1( 38,  37,  39, 48),
    IO1( 39,  38,  40, 49),
    IO1( 40,  39,  41, 50),
    IO1( 41,  40,  42, 51),
    IO1( 42,  41,  43, 52),
    IO1( 43,  42,  44, 53),
    IO1( 44,  43,  45, 54),
    IO1( 45,  44, NON, 55),  // [gap: GND apin46 die pad 56, VDD IO apin47 die pad 57]
    // ── die pads 58–61 (apin 48–51) ─────────────────────────────────────────
    IO1( 48, NON,  49, 58),
    IO1( 49,  48,  50, 59),
    IO1( 50,  49,  51, 60),
    IO1( 51,  50, NON, 61),  // [gap: GND apin53 die pad 62, PWR Aux apin52 die pad 63]
    // ── die pads 64–69 (apin 54–59) ─────────────────────────────────────────
    IO1( 54, NON,  55, 64),
    IO1( 55,  54,  56, 65),
    IO1( 56,  55,  57, 66),
    IO1( 57,  56,  58, 67),
    IO1( 58,  57,  59, 68),
    IO1( 59,  58, NON, 69),  // [gap: ring wraps through GND apin61 die pad 72]
    // ── VDD/PWR ──────────────────────────────────────────────────────────────
    // groupId 1 = PWR_AUX (pass if ≥1), groupId 2 = VDDIO (pass if ≥1)
    { .adapterPin =  9, .gndPin = GND, .prevPin =  8, .nextPin = NON, .diePad = 17, .strategy = TestStrategy::STANDARD,   .padType = PadType::VDDIO,    .groupId = 2, .settleMs = 2, .thresholds = &kThreshPwrPm1    },  // die pad 17 VDD IO
    { .adapterPin = 17, .gndPin = GND, .prevPin = 16, .nextPin = NON, .diePad = 25, .strategy = TestStrategy::SKIP_SENSE, .padType = PadType::PWR_AUX,  .groupId = 1, .settleMs = 2, .thresholds = &kThreshPwrAuxPm1 },  // die pad 25 PWR Aux
    { .adapterPin = 25, .gndPin = GND, .prevPin = NON, .nextPin = NON, .diePad = 34, .strategy = TestStrategy::STANDARD,  .padType = PadType::VDD_CORE, .groupId = 0, .settleMs = 2, .thresholds = &kThreshPwrPm1    },  // die pad 34 VDD Core
    { .adapterPin = 27, .gndPin = GND, .prevPin = NON, .nextPin =  28, .diePad = 36, .strategy = TestStrategy::STANDARD,  .padType = PadType::VDDIO,    .groupId = 2, .settleMs = 2, .thresholds = &kThreshPwrPm1    },  // die pad 36 VDD IO
    { .adapterPin = 47, .gndPin = GND, .prevPin = NON, .nextPin =  48, .diePad = 57, .strategy = TestStrategy::STANDARD,  .padType = PadType::VDDIO,    .groupId = 2, .settleMs = 2, .thresholds = &kThreshPwrPm1    },  // die pad 57 VDD IO
    { .adapterPin = 60, .gndPin = GND, .prevPin = NON, .nextPin = NON, .diePad = 71, .strategy = TestStrategy::STANDARD,  .padType = PadType::VDD_CORE, .groupId = 0, .settleMs = 2, .thresholds = &kThreshPwrPm1    },  // die pad 71 VDD Core
    { .adapterPin = 62, .gndPin = GND, .prevPin = NON, .nextPin =  63, .diePad = 73, .strategy = TestStrategy::STANDARD,  .padType = PadType::VDDIO,    .groupId = 2, .settleMs = 2, .thresholds = &kThreshPwrPm1    },  // die pad 73 VDD IO
};
static_assert(sizeof(_pm1Cases) / sizeof(_pm1Cases[0]) == 63, "pm1 case count mismatch");

// — Pad map 2: Mezzanine70 v2 ——————————————————————————————————————————————
// 56 IO + 8 VDD/PWR = 64 cases. Identical to pm1 except apin34/die pad 43 is connected.

static const TestCase _pm2Cases[] = {
    // ── die pads  0– 7 (apin 63–70) ─────────────────────────────────────────
    IO2( 63, NON,  64,  0),
    IO2( 64,  63,  65,  1),
    IO2( 65,  64,  66,  2),
    IO2( 66,  65,  67,  3),
    IO2( 67,  66,  68,  4),
    IO2( 68,  67,  69,  5),
    IO2( 69,  68,  70,  6),
    IO2( 70,  69,   1,  7),  // [gap: GND die pad 8]
    // ── die pads  9–16 (apin  1– 8) ─────────────────────────────────────────
    IO2(  1,  70,   2,  9),
    IO2(  2,   1,   3, 10),
    IO2(  3,   2,   4, 11),
    IO2(  4,   3,   5, 12),
    IO2(  5,   4,   6, 13),
    IO2(  6,   5,   7, 14),
    IO2(  7,   6,   8, 15),
    IO2(  8,   7, NON, 16),  // [gap: VDD IO apin9 die pad 17, GND apin10 die pad 18]
    // ── die pads 19–24 (apin 11–16) ─────────────────────────────────────────
    IO2( 11, NON,  12, 19),
    IO2( 12,  11,  13, 20),
    IO2( 13,  12,  14, 21),
    IO2( 14,  13,  15, 22),
    IO2( 15,  14,  16, 23),
    IO2( 16,  15, NON, 24),  // [gap: PWR Aux apin17 die pad 25, GND apin18 die pad 26]
    // ── die pads 27–32 (apin 19–24) ─────────────────────────────────────────
    IO2( 19, NON,  20, 27),
    IO2( 20,  19,  21, 28),
    IO2( 21,  20,  22, 29),
    IO2( 22,  21,  23, 30),
    IO2( 23,  22,  24, 31),
    IO2( 24,  23, NON, 32),  // [gap: VDD Core apin25 die pad 34, VDD IO apin27 die pad 36]
    // ── die pads 37–44 (apin 28–35) — apin34/die pad 43 connected in v2 ─────
    IO2( 28, NON,  29, 37),
    IO2( 29,  28,  30, 38),
    IO2( 30,  29,  31, 39),
    IO2( 31,  30,  32, 40),
    IO2( 32,  31,  33, 41),
    IO2( 33,  32,  34, 42),
    IO2( 34,  33,  35, 43),  // die pad 43 / apin34 — connected in v2
    IO2( 35,  34,  36, 44),
    // ── die pads 46–55 (apin 36–45) ─────────────────────────────────────────
    IO2( 36,  35,  37, 46),
    IO2( 37,  36,  38, 47),
    IO2( 38,  37,  39, 48),
    IO2( 39,  38,  40, 49),
    IO2( 40,  39,  41, 50),
    IO2( 41,  40,  42, 51),
    IO2( 42,  41,  43, 52),
    IO2( 43,  42,  44, 53),
    IO2( 44,  43,  45, 54),
    IO2( 45,  44, NON, 55),  // [gap: GND apin46 die pad 56, VDD IO apin47 die pad 57]
    // ── die pads 58–61 (apin 48–51) ─────────────────────────────────────────
    IO2( 48, NON,  49, 58),
    IO2( 49,  48,  50, 59),
    IO2( 50,  49,  51, 60),
    IO2( 51,  50, NON, 61),  // [gap: GND apin53 die pad 62, PWR Aux apin52 die pad 63]
    // ── die pads 64–69 (apin 54–59) ─────────────────────────────────────────
    IO2( 54, NON,  55, 64),
    IO2( 55,  54,  56, 65),
    IO2( 56,  55,  57, 66),
    IO2( 57,  56,  58, 67),
    IO2( 58,  57,  59, 68),
    IO2( 59,  58, NON, 69),  // [gap: ring wraps through GND apin61 die pad 72]
    // ── VDD/PWR ──────────────────────────────────────────────────────────────
    { .adapterPin =  9, .gndPin = GND, .prevPin =  8, .nextPin = NON, .diePad = 17, .strategy = TestStrategy::STANDARD,   .padType = PadType::VDDIO,    .groupId = 2, .settleMs = 2, .thresholds = &kThreshPwrPm2    },  // die pad 17 VDD IO
    { .adapterPin = 17, .gndPin = GND, .prevPin = 16, .nextPin = NON, .diePad = 25, .strategy = TestStrategy::SKIP_SENSE, .padType = PadType::PWR_AUX,  .groupId = 1, .settleMs = 2, .thresholds = &kThreshPwrAuxPm2 },  // die pad 25 PWR Aux
    { .adapterPin = 25, .gndPin = GND, .prevPin = NON, .nextPin = NON, .diePad = 34, .strategy = TestStrategy::STANDARD,  .padType = PadType::VDD_CORE, .groupId = 0, .settleMs = 2, .thresholds = &kThreshPwrPm2    },  // die pad 34 VDD Core
    { .adapterPin = 27, .gndPin = GND, .prevPin = NON, .nextPin =  28, .diePad = 36, .strategy = TestStrategy::STANDARD,  .padType = PadType::VDDIO,    .groupId = 2, .settleMs = 2, .thresholds = &kThreshPwrPm2    },  // die pad 36 VDD IO
    { .adapterPin = 47, .gndPin = GND, .prevPin = NON, .nextPin =  48, .diePad = 57, .strategy = TestStrategy::STANDARD,  .padType = PadType::VDDIO,    .groupId = 2, .settleMs = 2, .thresholds = &kThreshPwrPm2    },  // die pad 57 VDD IO
    { .adapterPin = 52, .gndPin = GND, .prevPin = NON, .nextPin =  54, .diePad = 63, .strategy = TestStrategy::SKIP_SENSE, .padType = PadType::PWR_AUX, .groupId = 1, .settleMs = 2, .thresholds = &kThreshPwrAuxPm2 },  // die pad 63 PWR Aux
    { .adapterPin = 60, .gndPin = GND, .prevPin = NON, .nextPin = NON, .diePad = 71, .strategy = TestStrategy::STANDARD,  .padType = PadType::VDD_CORE, .groupId = 0, .settleMs = 2, .thresholds = &kThreshPwrPm2    },  // die pad 71 VDD Core
    { .adapterPin = 62, .gndPin = GND, .prevPin = NON, .nextPin =  63, .diePad = 73, .strategy = TestStrategy::STANDARD,  .padType = PadType::VDDIO,    .groupId = 2, .settleMs = 2, .thresholds = &kThreshPwrPm2    },  // die pad 73 VDD IO
};
static_assert(sizeof(_pm2Cases) / sizeof(_pm2Cases[0]) == 64, "pm2 case count mismatch");

// — Pad map 3: 1x0p5 Mezzanine70 v1 ————————————————————————————————————————
// 56 IO + 8 VDD/PWR = 64 cases. Source: docs/DUT_PADMAP_1X0P5.md.
// GND adapter pins (equivalent): 10, 18, 26, 46, 53, 61. Using adapter pin 26 (die pad 41) as gndPin.

static const TestCase _pm3Cases[] = {
    // ── die pads  0– 3 (apin 63–66) — clk, rst_n, bidir_0–1 ─────────────────
    IO3( 63, NON,  64,  0),  // clk
    IO3( 64,  63,  65,  1),  // rst_n
    IO3( 65,  64,  66,  2),  // bidir_0
    IO3( 66,  65, NON,  3),  // bidir_1   [gap: GND apin61 die pad 4, VDD CORE 1 apin60 die pad 5]
    // ── die pads  6– 9, 10–17 (apin 67–70, 1–8) — bidir_2–13 ───────────────
    IO3( 67, NON,  68,  6),  // bidir_2
    IO3( 68,  67,  69,  7),  // bidir_3
    IO3( 69,  68,  70,  8),  // bidir_4
    IO3( 70,  69,   1,  9),  // bidir_5
    IO3(  1,  70,   2, 10),  // bidir_6
    IO3(  2,   1,   3, 11),  // bidir_7
    IO3(  3,   2,   4, 12),  // bidir_8
    IO3(  4,   3,   5, 13),  // bidir_9
    IO3(  5,   4,   6, 14),  // bidir_10
    IO3(  6,   5,   7, 15),  // bidir_11
    IO3(  7,   6,   8, 16),  // bidir_12
    IO3(  8,   7, NON, 17),  // bidir_13  [gap: GND IO apin10 die pad 18, VDD IO 0 apin9 die pad 19]
    // ── die pads 20–23 (apin 11–14) — bidir_14–17 ───────────────────────────
    IO3( 11, NON,  12, 20),  // bidir_14
    IO3( 12,  11,  13, 21),  // bidir_15
    IO3( 13,  12,  14, 22),  // bidir_16
    IO3( 14,  13, NON, 23),  // bidir_17  [gap: GND apin18 die pad 24, PWR Aux 0 apin17 die pad 25]
    // ── die pads 26–27, 28–33 (apin 15–16, 19–24) — bidir_18–25 ─────────────
    IO3( 15, NON,  16, 26),  // bidir_18
    IO3( 16,  15,  19, 27),  // bidir_19
    IO3( 19,  16,  20, 28),  // bidir_20
    IO3( 20,  19,  21, 29),  // bidir_21
    IO3( 21,  20,  22, 30),  // bidir_22
    IO3( 22,  21,  23, 31),  // bidir_23
    IO3( 23,  22,  24, 32),  // bidir_24
    IO3( 24,  23, NON, 33),  // bidir_25  [gap: GND(no adapter pin) die pad 34, VDD IO 1 apin27 die pad 35]
    // ── die pads 36–39 (apin 28–31) — bidir_26–29 ───────────────────────────
    IO3( 28, NON,  29, 36),  // bidir_26
    IO3( 29,  28,  30, 37),  // bidir_27
    IO3( 30,  29,  31, 38),  // bidir_28
    IO3( 31,  30, NON, 39),  // bidir_29  [gap: VDD CORE apin25 die pad 40, GND apin26 die pad 41]
    // ── die pads 42–53 (apin 32–43) — bidir_30–41 ───────────────────────────
    IO3( 32, NON,  33, 42),  // bidir_30
    IO3( 33,  32,  34, 43),  // bidir_31
    IO3( 34,  33,  35, 44),  // bidir_32
    IO3( 35,  34,  36, 45),  // bidir_33
    IO3( 36,  35,  37, 46),  // bidir_34
    IO3( 37,  36,  38, 47),  // bidir_35
    IO3( 38,  37,  39, 48),  // bidir_36
    IO3( 39,  38,  40, 49),  // bidir_37
    IO3( 40,  39,  41, 50),  // bidir_38
    IO3( 41,  40,  42, 51),  // bidir_39
    IO3( 42,  41,  43, 52),  // bidir_40
    IO3( 43,  42, NON, 53),  // bidir_41  [gap: VDD IO 2 apin47 die pad 54, GND apin46 die pad 55]
    // ── die pads 56–57, 58–59 (apin 44–45, 48–49) — bidir_42–45 ─────────────
    IO3( 44, NON,  45, 56),  // bidir_42
    IO3( 45,  44,  48, 57),  // bidir_43
    IO3( 48,  45,  49, 58),  // bidir_44
    IO3( 49,  48, NON, 59),  // bidir_45  [gap: PWR Aux 1 apin52 die pad 60, GND apin53 die pad 61]
    // ── die pads 62–65, 66–69 (apin 50–51, 54–55, 59–56) — an_0–3, in_0–3 ──
    IO3( 50, NON,  51, 62),  // an_0
    IO3( 51,  50,  54, 63),  // an_1
    IO3( 54,  51,  55, 64),  // an_2
    IO3( 55,  54,  59, 65),  // an_3
    IO3( 59,  55,  58, 66),  // in_0
    IO3( 58,  59,  57, 67),  // in_1
    IO3( 57,  58,  56, 68),  // in_2
    IO3( 56,  57, NON, 69),  // in_3      [gap: VDD IO 3 apin62 die pad 70, GND(no adapter pin) die pad 71]
    // ── VDD / PWR ─────────────────────────────────────────────────────────────
    { .adapterPin =  9, .gndPin = GND3, .prevPin = NON, .nextPin =  11, .diePad = 19, .strategy = TestStrategy::STANDARD,   .padType = PadType::VDDIO,    .groupId = 2, .settleMs = 2, .thresholds = &kThreshPwrPm3    },  // die pad 19 VDD IO 0
    { .adapterPin = 17, .gndPin = GND3, .prevPin = NON, .nextPin =  15, .diePad = 25, .strategy = TestStrategy::SKIP_SENSE, .padType = PadType::PWR_AUX,  .groupId = 1, .settleMs = 2, .thresholds = &kThreshPwrAuxPm3 },  // die pad 25 PWR Aux 0
    { .adapterPin = 25, .gndPin = GND3, .prevPin =  31, .nextPin = NON, .diePad = 40, .strategy = TestStrategy::STANDARD,   .padType = PadType::VDD_CORE, .groupId = 0, .settleMs = 2, .thresholds = &kThreshPwrPm3    },  // die pad 40 VDD CORE
    { .adapterPin = 27, .gndPin = GND3, .prevPin = NON, .nextPin =  28, .diePad = 35, .strategy = TestStrategy::STANDARD,   .padType = PadType::VDDIO,    .groupId = 2, .settleMs = 2, .thresholds = &kThreshPwrPm3    },  // die pad 35 VDD IO 1
    { .adapterPin = 47, .gndPin = GND3, .prevPin =  43, .nextPin = NON, .diePad = 54, .strategy = TestStrategy::STANDARD,   .padType = PadType::VDDIO,    .groupId = 2, .settleMs = 2, .thresholds = &kThreshPwrPm3    },  // die pad 54 VDD IO 2
    { .adapterPin = 52, .gndPin = GND3, .prevPin =  49, .nextPin = NON, .diePad = 60, .strategy = TestStrategy::SKIP_SENSE, .padType = PadType::PWR_AUX,  .groupId = 1, .settleMs = 2, .thresholds = &kThreshPwrAuxPm3 },  // die pad 60 PWR Aux 1
    { .adapterPin = 60, .gndPin = GND3, .prevPin = NON, .nextPin =  67, .diePad =  5, .strategy = TestStrategy::STANDARD,   .padType = PadType::VDD_CORE, .groupId = 0, .settleMs = 2, .thresholds = &kThreshPwrPm3    },  // die pad  5 VDD CORE 1
    { .adapterPin = 62, .gndPin = GND3, .prevPin =  56, .nextPin = NON, .diePad = 70, .strategy = TestStrategy::STANDARD,   .padType = PadType::VDDIO,    .groupId = 2, .settleMs = 2, .thresholds = &kThreshPwrPm3    },  // die pad 70 VDD IO 3
};
static_assert(sizeof(_pm3Cases) / sizeof(_pm3Cases[0]) == 64, "pm3 case count mismatch");

#undef IO1
#undef IO2
#undef IO3
#undef NON

// Groups for 1x1 pad maps (pm1, pm2)
static const PadGroup _groups[] = {
    { .id = 1, .minPass = 1, .name = "PWR_AUX" },  // 2 pins: die pads 25, 63 — pass if ≥1 bond
    { .id = 2, .minPass = 1, .name = "VDDIO"   },  // 4 pins: die pads 17, 36, 57, 73 — pass if ≥1 bond
};
static constexpr uint8_t GROUP_COUNT = sizeof(_groups) / sizeof(_groups[0]);

// Groups for 1x0p5 pad map (pm3)
static const PadGroup _pm3Groups[] = {
    { .id = 1, .minPass = 1, .name = "PWR_AUX" },  // 2 pins: die pads 25, 60 — pass if ≥1 bond
    { .id = 2, .minPass = 1, .name = "VDDIO"   },  // 4 pins: die pads 19, 35, 54, 70 — pass if ≥1 bond
};
static constexpr uint8_t PM3_GROUP_COUNT = sizeof(_pm3Groups) / sizeof(_pm3Groups[0]);

static const PadMap _maps[] = {
    {
        .id                 = 1,
        .name               = "1x1 Mezzanine70 v1",
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
        .name               = "1x1 Mezzanine70 v2",
        .cases              = _pm2Cases,
        .caseCount          = 64,
        .presencePadA       = 10,
        .presencePadB       = 53,
        .presenceThresholdV = 0.3f,
        .padGroups          = _groups,
        .padGroupCount      = GROUP_COUNT,
    },
    {
        .id                 = 3,
        .name               = "1x0p5 Mezzanine70 v1",
        .cases              = _pm3Cases,
        .caseCount          = 64,
        .presencePadA       = 10,
        .presencePadB       = 53,
        .presenceThresholdV = 0.3f,
        .padGroups          = _pm3Groups,
        .padGroupCount      = PM3_GROUP_COUNT,
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

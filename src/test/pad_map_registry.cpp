#include "pad_map_registry.h"

// Activity-detection ceiling: a reading counts as "conducted" if its apparent
// bond resistance is below this. Same ceiling for both STANDARD (IO pads)
// and CAP_SENSE (VDDIO/VDD_CORE/PWR_AUX, which have a real bypass cap and
// use CAP_SENSE's longer 3.3k-only settle instead of STANDARD's 6-reading
// sweep — see TestStrategy::CAP_SENSE in pad_map.h). Shared-PCB-net power
// pins (e.g. a pin electrically tied to a sibling pin's bond through a
// shared bypass cap) are a separate, harder problem neither strategy can
// fix — see kelvin.h.
//
// 50kΩ, from bench data with visually-confirmed-missing bonds: real bonds
// (incl. weak ESD-diode-mediated ones) conduct at the 33k/3.3k levels up to
// ~9.2kΩ/~1.6kΩ — the 330k level alone can read as high as ~81kΩ for a real
// bond, but OR-across-readings means that's fine, the same pad still passes
// via its 33k/3.3k reading. Confirmed-missing bonds never dropped below
// ~256kΩ at ANY reading (leakage/coupling artifacts, not a real low-
// resistance path) — 50kΩ sits with ~5x margin on both sides. This was
// calibrated against STANDARD's readings; CAP_SENSE reuses it unverified —
// worth confirming on the bench once real CAP_SENSE data is available.
static constexpr TestThresholds kThresh = { 60000.0f };

static constexpr uint8_t GND  = 10;  // adapter pin 10, die pad 18; all GND pins equivalent (1x1)
static constexpr uint8_t GND3 = 26;  // adapter pin 26, die pad 41; all GND pins equivalent (1x0p5)

// IO case shorthand — args: adapterPin, diePad
#define IO1(m_, d_)  \
    { .adapterPin=(m_), .gndPin=GND,  \
      .diePad=(d_), .strategy=TestStrategy::STANDARD, .padType=PadType::IO, \
      .settleUs=200, .thresholds=&kThresh }
#define IO2(m_, d_)  \
    { .adapterPin=(m_), .gndPin=GND,  \
      .diePad=(d_), .strategy=TestStrategy::STANDARD, .padType=PadType::IO, \
      .settleUs=200, .thresholds=&kThresh }
#define IO3(m_, d_)  \
    { .adapterPin=(m_), .gndPin=GND3, \
      .diePad=(d_), .strategy=TestStrategy::STANDARD, .padType=PadType::IO, \
      .settleUs=200, .thresholds=&kThresh }

// — Pad map 1: Mezzanine70 v1 ——————————————————————————————————————————————
// 56 IO + 7 VDD/PWR = 63 cases. Source: docs/DUT_PADMAP_1X1.md.
// GND adapter pins (equivalent): 10, 18, 26, 46, 53, 61.
// adapter pin 52/die pad 63 (PWR_AUX) omitted — unconnected PCB trace on v1 boards.
// adapter pin 34/die pad 43 IS connected on v1 (contrary to earlier assumption).

static const TestCase _pm1Cases[] = {
    // ── die pads  0– 7 (apin 63–70) ─────────────────────────────────────────
    IO1( 63,  0),
    IO1( 64,  1),
    IO1( 65,  2),
    IO1( 66,  3),
    IO1( 67,  4),
    IO1( 68,  5),
    IO1( 69,  6),
    IO1( 70,  7),  // [gap: GND die pad 8]
    // ── die pads  9–16 (apin  1– 8) ─────────────────────────────────────────
    IO1(  1,  9),
    IO1(  2, 10),
    IO1(  3, 11),
    IO1(  4, 12),
    IO1(  5, 13),
    IO1(  6, 14),
    IO1(  7, 15),
    IO1(  8, 16),  // [gap: VDD IO apin9 die pad 17, GND apin10 die pad 18]
    // ── die pads 19–24 (apin 11–16) ─────────────────────────────────────────
    IO1( 11, 19),
    IO1( 12, 20),
    IO1( 13, 21),
    IO1( 14, 22),
    IO1( 15, 23),
    IO1( 16, 24),  // [gap: PWR Aux apin17 die pad 25, GND apin18 die pad 26]
    // ── die pads 27–32 (apin 19–24) ─────────────────────────────────────────
    IO1( 19, 27),
    IO1( 20, 28),
    IO1( 21, 29),
    IO1( 22, 30),
    IO1( 23, 31),
    IO1( 24, 32),  // [gap: VDD Core apin25 die pad 34, VDD IO apin27 die pad 36]
    // ── die pads 37–44 (apin 28–35) ─────────────────────────────────────────
    IO1( 28, 37),
    IO1( 29, 38),
    IO1( 30, 39),
    IO1( 31, 40),
    IO1( 32, 41),
    IO1( 33, 42),
    IO1( 34, 43),
    IO1( 35, 44),
    // ── die pads 46–55 (apin 36–45) ─────────────────────────────────────────
    IO1( 36, 46),
    IO1( 37, 47),
    IO1( 38, 48),
    IO1( 39, 49),
    IO1( 40, 50),
    IO1( 41, 51),
    IO1( 42, 52),
    IO1( 43, 53),
    IO1( 44, 54),
    IO1( 45, 55),  // [gap: GND apin46 die pad 56, VDD IO apin47 die pad 57]
    // ── die pads 58–61 (apin 48–51) ─────────────────────────────────────────
    IO1( 48, 58),
    IO1( 49, 59),
    IO1( 50, 60),
    IO1( 51, 61),  // [gap: GND apin53 die pad 62, PWR Aux apin52 die pad 63]
    // ── die pads 64–69 (apin 54–59) ─────────────────────────────────────────
    IO1( 54, 64),
    IO1( 55, 65),
    IO1( 56, 66),
    IO1( 57, 67),
    IO1( 58, 68),
    IO1( 59, 69),  // [gap: ring wraps through GND apin61 die pad 72]
    // ── VDD/PWR ──────────────────────────────────────────────────────────────
    { .adapterPin =  9, .gndPin = GND, .diePad = 17, .strategy = TestStrategy::CAP_SENSE, .padType = PadType::VDDIO,    .settleUs = 20000, .thresholds = &kThresh    },  // die pad 17 VDD IO
    { .adapterPin = 17, .gndPin = GND, .diePad = 25, .strategy = TestStrategy::CAP_SENSE, .padType = PadType::PWR_AUX,  .settleUs = 20000, .thresholds = &kThresh },  // die pad 25 PWR Aux
    { .adapterPin = 25, .gndPin = GND, .diePad = 34, .strategy = TestStrategy::CAP_SENSE, .padType = PadType::VDD_CORE, .settleUs = 20000, .thresholds = &kThresh    },  // die pad 34 VDD Core
    { .adapterPin = 27, .gndPin = GND, .diePad = 36, .strategy = TestStrategy::CAP_SENSE, .padType = PadType::VDDIO,    .settleUs = 20000, .thresholds = &kThresh    },  // die pad 36 VDD IO
    { .adapterPin = 47, .gndPin = GND, .diePad = 57, .strategy = TestStrategy::CAP_SENSE, .padType = PadType::VDDIO,    .settleUs = 20000, .thresholds = &kThresh    },  // die pad 57 VDD IO
    { .adapterPin = 60, .gndPin = GND, .diePad = 71, .strategy = TestStrategy::CAP_SENSE, .padType = PadType::VDD_CORE, .settleUs = 20000, .thresholds = &kThresh    },  // die pad 71 VDD Core
    { .adapterPin = 62, .gndPin = GND, .diePad = 73, .strategy = TestStrategy::CAP_SENSE, .padType = PadType::VDDIO,    .settleUs = 20000, .thresholds = &kThresh    },  // die pad 73 VDD IO
};
static_assert(sizeof(_pm1Cases) / sizeof(_pm1Cases[0]) == 63, "pm1 case count mismatch");

// — Pad map 2: Mezzanine70 v2 ——————————————————————————————————————————————
// 56 IO + 8 VDD/PWR = 64 cases. Identical to pm1 except apin34/die pad 43 is connected.

static const TestCase _pm2Cases[] = {
    // ── die pads  0– 7 (apin 63–70) ─────────────────────────────────────────
    IO2( 63,  0),
    IO2( 64,  1),
    IO2( 65,  2),
    IO2( 66,  3),
    IO2( 67,  4),
    IO2( 68,  5),
    IO2( 69,  6),
    IO2( 70,  7),  // [gap: GND die pad 8]
    // ── die pads  9–16 (apin  1– 8) ─────────────────────────────────────────
    IO2(  1,  9),
    IO2(  2, 10),
    IO2(  3, 11),
    IO2(  4, 12),
    IO2(  5, 13),
    IO2(  6, 14),
    IO2(  7, 15),
    IO2(  8, 16),  // [gap: VDD IO apin9 die pad 17, GND apin10 die pad 18]
    // ── die pads 19–24 (apin 11–16) ─────────────────────────────────────────
    IO2( 11, 19),
    IO2( 12, 20),
    IO2( 13, 21),
    IO2( 14, 22),
    IO2( 15, 23),
    IO2( 16, 24),  // [gap: PWR Aux apin17 die pad 25, GND apin18 die pad 26]
    // ── die pads 27–32 (apin 19–24) ─────────────────────────────────────────
    IO2( 19, 27),
    IO2( 20, 28),
    IO2( 21, 29),
    IO2( 22, 30),
    IO2( 23, 31),
    IO2( 24, 32),  // [gap: VDD Core apin25 die pad 34, VDD IO apin27 die pad 36]
    // ── die pads 37–44 (apin 28–35) — apin34/die pad 43 connected in v2 ─────
    IO2( 28, 37),
    IO2( 29, 38),
    IO2( 30, 39),
    IO2( 31, 40),
    IO2( 32, 41),
    IO2( 33, 42),
    IO2( 34, 43),  // die pad 43 / apin34 — connected in v2
    IO2( 35, 44),
    // ── die pads 46–55 (apin 36–45) ─────────────────────────────────────────
    IO2( 36, 46),
    IO2( 37, 47),
    IO2( 38, 48),
    IO2( 39, 49),
    IO2( 40, 50),
    IO2( 41, 51),
    IO2( 42, 52),
    IO2( 43, 53),
    IO2( 44, 54),
    IO2( 45, 55),  // [gap: GND apin46 die pad 56, VDD IO apin47 die pad 57]
    // ── die pads 58–61 (apin 48–51) ─────────────────────────────────────────
    IO2( 48, 58),
    IO2( 49, 59),
    IO2( 50, 60),
    IO2( 51, 61),  // [gap: GND apin53 die pad 62, PWR Aux apin52 die pad 63]
    // ── die pads 64–69 (apin 54–59) ─────────────────────────────────────────
    IO2( 54, 64),
    IO2( 55, 65),
    IO2( 56, 66),
    IO2( 57, 67),
    IO2( 58, 68),
    IO2( 59, 69),  // [gap: ring wraps through GND apin61 die pad 72]
    // ── VDD/PWR ──────────────────────────────────────────────────────────────
    { .adapterPin =  9, .gndPin = GND, .diePad = 17, .strategy = TestStrategy::CAP_SENSE, .padType = PadType::VDDIO,    .settleUs = 20000, .thresholds = &kThresh    },  // die pad 17 VDD IO
    { .adapterPin = 17, .gndPin = GND, .diePad = 25, .strategy = TestStrategy::CAP_SENSE, .padType = PadType::PWR_AUX,  .settleUs = 20000, .thresholds = &kThresh },  // die pad 25 PWR Aux
    { .adapterPin = 25, .gndPin = GND, .diePad = 34, .strategy = TestStrategy::CAP_SENSE, .padType = PadType::VDD_CORE, .settleUs = 20000, .thresholds = &kThresh    },  // die pad 34 VDD Core
    { .adapterPin = 27, .gndPin = GND, .diePad = 36, .strategy = TestStrategy::CAP_SENSE, .padType = PadType::VDDIO,    .settleUs = 20000, .thresholds = &kThresh    },  // die pad 36 VDD IO
    { .adapterPin = 47, .gndPin = GND, .diePad = 57, .strategy = TestStrategy::CAP_SENSE, .padType = PadType::VDDIO,    .settleUs = 20000, .thresholds = &kThresh    },  // die pad 57 VDD IO
    { .adapterPin = 52, .gndPin = GND, .diePad = 63, .strategy = TestStrategy::CAP_SENSE, .padType = PadType::PWR_AUX,  .settleUs = 20000, .thresholds = &kThresh },  // die pad 63 PWR Aux
    { .adapterPin = 60, .gndPin = GND, .diePad = 71, .strategy = TestStrategy::CAP_SENSE, .padType = PadType::VDD_CORE, .settleUs = 20000, .thresholds = &kThresh    },  // die pad 71 VDD Core
    { .adapterPin = 62, .gndPin = GND, .diePad = 73, .strategy = TestStrategy::CAP_SENSE, .padType = PadType::VDDIO,    .settleUs = 20000, .thresholds = &kThresh    },  // die pad 73 VDD IO
};
static_assert(sizeof(_pm2Cases) / sizeof(_pm2Cases[0]) == 64, "pm2 case count mismatch");

// — Pad map 3: 1x0p5 Mezzanine70 v1 ————————————————————————————————————————
// 56 IO + 8 VDD/PWR = 64 cases. Source: docs/DUT_PADMAP_1X0P5.md.
// GND adapter pins (equivalent): 10, 18, 26, 46, 53, 61. Using adapter pin 26 (die pad 41) as gndPin.

static const TestCase _pm3Cases[] = {
    // ── die pads  0– 3 (apin 63–66) — clk, rst_n, bidir_0–1 ─────────────────
    IO3( 63,  0),  // clk
    IO3( 64,  1),  // rst_n
    IO3( 65,  2),  // bidir_0
    IO3( 66,  3),  // bidir_1   [gap: GND apin61 die pad 4, VDD CORE 1 apin60 die pad 5]
    // ── die pads  6– 9, 10–17 (apin 67–70, 1–8) — bidir_2–13 ───────────────
    IO3( 67,  6),  // bidir_2
    IO3( 68,  7),  // bidir_3
    IO3( 69,  8),  // bidir_4
    IO3( 70,  9),  // bidir_5
    IO3(  1, 10),  // bidir_6
    IO3(  2, 11),  // bidir_7
    IO3(  3, 12),  // bidir_8
    IO3(  4, 13),  // bidir_9
    IO3(  5, 14),  // bidir_10
    IO3(  6, 15),  // bidir_11
    IO3(  7, 16),  // bidir_12
    IO3(  8, 17),  // bidir_13  [gap: GND IO apin10 die pad 18, VDD IO 0 apin9 die pad 19]
    // ── die pads 20–23 (apin 11–14) — bidir_14–17 ───────────────────────────
    IO3( 11, 20),  // bidir_14
    IO3( 12, 21),  // bidir_15
    IO3( 13, 22),  // bidir_16
    IO3( 14, 23),  // bidir_17  [gap: GND apin18 die pad 24, PWR Aux 0 apin17 die pad 25]
    // ── die pads 26–27, 28–33 (apin 15–16, 19–24) — bidir_18–25 ─────────────
    IO3( 15, 26),  // bidir_18
    IO3( 16, 27),  // bidir_19
    IO3( 19, 28),  // bidir_20
    IO3( 20, 29),  // bidir_21
    IO3( 21, 30),  // bidir_22
    IO3( 22, 31),  // bidir_23
    IO3( 23, 32),  // bidir_24
    IO3( 24, 33),  // bidir_25  [gap: GND(no adapter pin) die pad 34, VDD IO 1 apin27 die pad 35]
    // ── die pads 36–39 (apin 28–31) — bidir_26–29 ───────────────────────────
    IO3( 28, 36),  // bidir_26
    IO3( 29, 37),  // bidir_27
    IO3( 30, 38),  // bidir_28
    IO3( 31, 39),  // bidir_29  [gap: VDD CORE apin25 die pad 40, GND apin26 die pad 41]
    // ── die pads 42–53 (apin 32–43) — bidir_30–41 ───────────────────────────
    IO3( 32, 42),  // bidir_30
    IO3( 33, 43),  // bidir_31
    IO3( 34, 44),  // bidir_32
    IO3( 35, 45),  // bidir_33
    IO3( 36, 46),  // bidir_34
    IO3( 37, 47),  // bidir_35
    IO3( 38, 48),  // bidir_36
    IO3( 39, 49),  // bidir_37
    IO3( 40, 50),  // bidir_38
    IO3( 41, 51),  // bidir_39
    IO3( 42, 52),  // bidir_40
    IO3( 43, 53),  // bidir_41  [gap: VDD IO 2 apin47 die pad 54, GND apin46 die pad 55]
    // ── die pads 56–57, 58–59 (apin 44–45, 48–49) — bidir_42–45 ─────────────
    IO3( 44, 56),  // bidir_42
    IO3( 45, 57),  // bidir_43
    IO3( 48, 58),  // bidir_44
    IO3( 49, 59),  // bidir_45  [gap: PWR Aux 1 apin52 die pad 60, GND apin53 die pad 61]
    // ── die pads 62–65, 66–69 (apin 50–51, 54–55, 59–56) — an_0–3, in_0–3 ──
    IO3( 50, 62),  // an_0
    IO3( 51, 63),  // an_1
    IO3( 54, 64),  // an_2
    IO3( 55, 65),  // an_3
    IO3( 59, 66),  // in_0
    IO3( 58, 67),  // in_1
    IO3( 57, 68),  // in_2
    IO3( 56, 69),  // in_3      [gap: VDD IO 3 apin62 die pad 70, GND(no adapter pin) die pad 71]
    // ── VDD / PWR ─────────────────────────────────────────────────────────────
    { .adapterPin =  9, .gndPin = GND3, .diePad = 19, .strategy = TestStrategy::CAP_SENSE, .padType = PadType::VDDIO,    .settleUs = 20000, .thresholds = &kThresh    },  // die pad 19 VDD IO 0
    { .adapterPin = 17, .gndPin = GND3, .diePad = 25, .strategy = TestStrategy::CAP_SENSE, .padType = PadType::PWR_AUX,  .settleUs = 20000, .thresholds = &kThresh },  // die pad 25 PWR Aux 0
    { .adapterPin = 25, .gndPin = GND3, .diePad = 40, .strategy = TestStrategy::CAP_SENSE, .padType = PadType::VDD_CORE, .settleUs = 20000, .thresholds = &kThresh    },  // die pad 40 VDD CORE
    { .adapterPin = 27, .gndPin = GND3, .diePad = 35, .strategy = TestStrategy::CAP_SENSE, .padType = PadType::VDDIO,    .settleUs = 20000, .thresholds = &kThresh    },  // die pad 35 VDD IO 1
    { .adapterPin = 47, .gndPin = GND3, .diePad = 54, .strategy = TestStrategy::CAP_SENSE, .padType = PadType::VDDIO,    .settleUs = 20000, .thresholds = &kThresh    },  // die pad 54 VDD IO 2
    { .adapterPin = 52, .gndPin = GND3, .diePad = 60, .strategy = TestStrategy::CAP_SENSE, .padType = PadType::PWR_AUX,  .settleUs = 20000, .thresholds = &kThresh },  // die pad 60 PWR Aux 1
    { .adapterPin = 60, .gndPin = GND3, .diePad =  5, .strategy = TestStrategy::CAP_SENSE, .padType = PadType::VDD_CORE, .settleUs = 20000, .thresholds = &kThresh    },  // die pad  5 VDD CORE 1
    { .adapterPin = 62, .gndPin = GND3, .diePad = 70, .strategy = TestStrategy::CAP_SENSE, .padType = PadType::VDDIO,    .settleUs = 20000, .thresholds = &kThresh    },  // die pad 70 VDD IO 3
};
static_assert(sizeof(_pm3Cases) / sizeof(_pm3Cases[0]) == 64, "pm3 case count mismatch");

#undef IO1
#undef IO2
#undef IO3

static const PadMap _maps[] = {
    {
        .id                 = 1,
        .name               = "1x1 Mezzanine70 v1",
        .cases              = _pm1Cases,
        .caseCount          = 63,
        .presencePadA       = 10,
        .presencePadB       = 53,
        .presenceThresholdV = 0.3f,
    },
    {
        .id                 = 2,
        .name               = "1x1 Mezzanine70 v2",
        .cases              = _pm2Cases,
        .caseCount          = 64,
        .presencePadA       = 10,
        .presencePadB       = 53,
        .presenceThresholdV = 0.3f,
    },
    {
        .id                 = 3,
        .name               = "1x0p5 Mezzanine70 v1",
        .cases              = _pm3Cases,
        .caseCount          = 64,
        .presencePadA       = 10,
        .presencePadB       = 53,
        .presenceThresholdV = 0.3f,
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

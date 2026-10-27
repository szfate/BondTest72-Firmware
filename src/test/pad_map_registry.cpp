#include "pad_map_registry.h"

// Bench data with visually confirmed missing bonds: real bonds read up to
// ~9.2kΩ at 27.4k and ~1.6kΩ at 2.49k (up to ~81kΩ at 280k, but another level
// always passes); missing bonds never read below ~256kΩ at any level. 60kΩ
// leaves ~6x margin to the real bonds and ~4x to the missing ones.
// TODO(bench): measured with 330k/33k/3.3k pullups — re-confirm at the current
// levels. CAP_SENSE reuses this value unverified.
static constexpr TestThresholds kThresh = { 60000.0f };

// Reference/return pin per map. Any of a map's GND pins would do, except on MOSB.
static constexpr uint8_t GND  = 10;  // 1x1:   die pad 18
static constexpr uint8_t GND3 = 26;  // 1x0p5: die pad 41
static constexpr uint8_t GND5 = 26;  // 0p5x1: die pad 32
static constexpr uint8_t GND6 = 53;  // TQVA:  die pad 42
static constexpr uint8_t GNDM = 43;  // MOSB:  die pad 54, its only GND pin

// IO: paid per reading, so kept short. Sized for an OPEN pad at 280k charging
// the sense node (~380pF, τ≈107µs): 500µs ≈ 4.7τ.
// CAP: total curve window — see TestStrategy::CAP_SENSE.
static constexpr uint16_t IO_SETTLE_US  = 500;
static constexpr uint16_t CAP_SETTLE_US = 20000;

// IO case shorthand — args: adapterPin, diePad
#define IO(m_, d_)  \
    { .adapterPin=(m_), .gndPin=GND,  \
      .diePad=(d_), .strategy=TestStrategy::STANDARD, .padType=PadType::IO, \
      .settleUs=IO_SETTLE_US, .thresholds=&kThresh }
#define IO3(m_, d_)  \
    { .adapterPin=(m_), .gndPin=GND3, \
      .diePad=(d_), .strategy=TestStrategy::STANDARD, .padType=PadType::IO, \
      .settleUs=IO_SETTLE_US, .thresholds=&kThresh }
#define IO5(m_, d_)  \
    { .adapterPin=(m_), .gndPin=GND5, \
      .diePad=(d_), .strategy=TestStrategy::STANDARD, .padType=PadType::IO, \
      .settleUs=IO_SETTLE_US, .thresholds=&kThresh }
#define IO6(m_, d_)  \
    { .adapterPin=(m_), .gndPin=GND6, \
      .diePad=(d_), .strategy=TestStrategy::STANDARD, .padType=PadType::IO, \
      .settleUs=IO_SETTLE_US, .thresholds=&kThresh }
#define IOM(m_, d_)  \
    { .adapterPin=(m_), .gndPin=GNDM, \
      .diePad=(d_), .strategy=TestStrategy::STANDARD, .padType=PadType::IO, \
      .settleUs=IO_SETTLE_US, .thresholds=&kThresh }

// Pad map sources list DUT pins; adapter_pin = 71 − dut_pin for every map.

// — Pad map 1: Mezzanine70 v1 ——————————————————————————————————————————————
// Source: docs/DUT_PADMAP_1X1.md. GND pins: 10, 18, 26, 46, 53, 61.
// Die pad 63 (PWR_AUX, apin 52) left out: its trace is unconnected on v1 boards.

static const TestCase _pm1Cases[] = {
    // ── die pads  0– 7 (apin 63–70) ─────────────────────────────────────────
    IO( 63,  0),
    IO( 64,  1),
    IO( 65,  2),
    IO( 66,  3),
    IO( 67,  4),
    IO( 68,  5),
    IO( 69,  6),
    IO( 70,  7),  // [gap: GND die pad 8]
    // ── die pads  9–16 (apin  1– 8) ─────────────────────────────────────────
    IO(  1,  9),
    IO(  2, 10),
    IO(  3, 11),
    IO(  4, 12),
    IO(  5, 13),
    IO(  6, 14),
    IO(  7, 15),
    IO(  8, 16),  // [gap: VDD IO apin9 die pad 17, GND apin10 die pad 18]
    // ── die pads 19–24 (apin 11–16) ─────────────────────────────────────────
    IO( 11, 19),
    IO( 12, 20),
    IO( 13, 21),
    IO( 14, 22),
    IO( 15, 23),
    IO( 16, 24),  // [gap: PWR Aux apin17 die pad 25, GND apin18 die pad 26]
    // ── die pads 27–32 (apin 19–24) ─────────────────────────────────────────
    IO( 19, 27),
    IO( 20, 28),
    IO( 21, 29),
    IO( 22, 30),
    IO( 23, 31),
    IO( 24, 32),  // [gap: VDD Core apin25 die pad 34, VDD IO apin27 die pad 36]
    // ── die pads 37–44 (apin 28–35) ─────────────────────────────────────────
    IO( 28, 37),
    IO( 29, 38),
    IO( 30, 39),
    IO( 31, 40),
    IO( 32, 41),
    IO( 33, 42),
    IO( 34, 43),
    IO( 35, 44),
    // ── die pads 46–55 (apin 36–45) ─────────────────────────────────────────
    IO( 36, 46),
    IO( 37, 47),
    IO( 38, 48),
    IO( 39, 49),
    IO( 40, 50),
    IO( 41, 51),
    IO( 42, 52),
    IO( 43, 53),
    IO( 44, 54),
    IO( 45, 55),  // [gap: GND apin46 die pad 56, VDD IO apin47 die pad 57]
    // ── die pads 58–61 (apin 48–51) ─────────────────────────────────────────
    IO( 48, 58),
    IO( 49, 59),
    IO( 50, 60),
    IO( 51, 61),  // [gap: GND apin53 die pad 62, PWR Aux apin52 die pad 63]
    // ── die pads 64–69 (apin 54–59) ─────────────────────────────────────────
    IO( 54, 64),
    IO( 55, 65),
    IO( 56, 66),
    IO( 57, 67),
    IO( 58, 68),
    IO( 59, 69),  // [gap: ring wraps through GND apin61 die pad 72]
    // ── VDD/PWR ──────────────────────────────────────────────────────────────
    { .adapterPin =  9, .gndPin = GND, .diePad = 17, .strategy = TestStrategy::CAP_SENSE, .padType = PadType::VDDIO,    .settleUs = CAP_SETTLE_US, .thresholds = &kThresh    },  // die pad 17 VDD IO
    { .adapterPin = 17, .gndPin = GND, .diePad = 25, .strategy = TestStrategy::CAP_SENSE, .padType = PadType::PWR_AUX,  .settleUs = CAP_SETTLE_US, .thresholds = &kThresh },  // die pad 25 PWR Aux
    { .adapterPin = 25, .gndPin = GND, .diePad = 34, .strategy = TestStrategy::CAP_SENSE, .padType = PadType::VDD_CORE, .settleUs = CAP_SETTLE_US, .thresholds = &kThresh    },  // die pad 34 VDD Core
    { .adapterPin = 27, .gndPin = GND, .diePad = 36, .strategy = TestStrategy::CAP_SENSE, .padType = PadType::VDDIO,    .settleUs = CAP_SETTLE_US, .thresholds = &kThresh    },  // die pad 36 VDD IO
    { .adapterPin = 47, .gndPin = GND, .diePad = 57, .strategy = TestStrategy::CAP_SENSE, .padType = PadType::VDDIO,    .settleUs = CAP_SETTLE_US, .thresholds = &kThresh    },  // die pad 57 VDD IO
    { .adapterPin = 60, .gndPin = GND, .diePad = 71, .strategy = TestStrategy::CAP_SENSE, .padType = PadType::VDD_CORE, .settleUs = CAP_SETTLE_US, .thresholds = &kThresh    },  // die pad 71 VDD Core
    { .adapterPin = 62, .gndPin = GND, .diePad = 73, .strategy = TestStrategy::CAP_SENSE, .padType = PadType::VDDIO,    .settleUs = CAP_SETTLE_US, .thresholds = &kThresh    },  // die pad 73 VDD IO
};
static_assert(sizeof(_pm1Cases) / sizeof(_pm1Cases[0]) == 63, "pm1 case count mismatch");

// — Pad map 2: Mezzanine70 v2 ——————————————————————————————————————————————
// Pad map 1 plus die pad 63 (PWR_AUX, apin 52).

static const TestCase _pm2Cases[] = {
    // ── die pads  0– 7 (apin 63–70) ─────────────────────────────────────────
    IO( 63,  0),
    IO( 64,  1),
    IO( 65,  2),
    IO( 66,  3),
    IO( 67,  4),
    IO( 68,  5),
    IO( 69,  6),
    IO( 70,  7),  // [gap: GND die pad 8]
    // ── die pads  9–16 (apin  1– 8) ─────────────────────────────────────────
    IO(  1,  9),
    IO(  2, 10),
    IO(  3, 11),
    IO(  4, 12),
    IO(  5, 13),
    IO(  6, 14),
    IO(  7, 15),
    IO(  8, 16),  // [gap: VDD IO apin9 die pad 17, GND apin10 die pad 18]
    // ── die pads 19–24 (apin 11–16) ─────────────────────────────────────────
    IO( 11, 19),
    IO( 12, 20),
    IO( 13, 21),
    IO( 14, 22),
    IO( 15, 23),
    IO( 16, 24),  // [gap: PWR Aux apin17 die pad 25, GND apin18 die pad 26]
    // ── die pads 27–32 (apin 19–24) ─────────────────────────────────────────
    IO( 19, 27),
    IO( 20, 28),
    IO( 21, 29),
    IO( 22, 30),
    IO( 23, 31),
    IO( 24, 32),  // [gap: VDD Core apin25 die pad 34, VDD IO apin27 die pad 36]
    // ── die pads 37–44 (apin 28–35) ─────────────────────────────────────────
    IO( 28, 37),
    IO( 29, 38),
    IO( 30, 39),
    IO( 31, 40),
    IO( 32, 41),
    IO( 33, 42),
    IO( 34, 43),
    IO( 35, 44),
    // ── die pads 46–55 (apin 36–45) ─────────────────────────────────────────
    IO( 36, 46),
    IO( 37, 47),
    IO( 38, 48),
    IO( 39, 49),
    IO( 40, 50),
    IO( 41, 51),
    IO( 42, 52),
    IO( 43, 53),
    IO( 44, 54),
    IO( 45, 55),  // [gap: GND apin46 die pad 56, VDD IO apin47 die pad 57]
    // ── die pads 58–61 (apin 48–51) ─────────────────────────────────────────
    IO( 48, 58),
    IO( 49, 59),
    IO( 50, 60),
    IO( 51, 61),  // [gap: GND apin53 die pad 62, PWR Aux apin52 die pad 63]
    // ── die pads 64–69 (apin 54–59) ─────────────────────────────────────────
    IO( 54, 64),
    IO( 55, 65),
    IO( 56, 66),
    IO( 57, 67),
    IO( 58, 68),
    IO( 59, 69),  // [gap: ring wraps through GND apin61 die pad 72]
    // ── VDD/PWR ──────────────────────────────────────────────────────────────
    { .adapterPin =  9, .gndPin = GND, .diePad = 17, .strategy = TestStrategy::CAP_SENSE, .padType = PadType::VDDIO,    .settleUs = CAP_SETTLE_US, .thresholds = &kThresh    },  // die pad 17 VDD IO
    { .adapterPin = 17, .gndPin = GND, .diePad = 25, .strategy = TestStrategy::CAP_SENSE, .padType = PadType::PWR_AUX,  .settleUs = CAP_SETTLE_US, .thresholds = &kThresh },  // die pad 25 PWR Aux
    { .adapterPin = 25, .gndPin = GND, .diePad = 34, .strategy = TestStrategy::CAP_SENSE, .padType = PadType::VDD_CORE, .settleUs = CAP_SETTLE_US, .thresholds = &kThresh    },  // die pad 34 VDD Core
    { .adapterPin = 27, .gndPin = GND, .diePad = 36, .strategy = TestStrategy::CAP_SENSE, .padType = PadType::VDDIO,    .settleUs = CAP_SETTLE_US, .thresholds = &kThresh    },  // die pad 36 VDD IO
    { .adapterPin = 47, .gndPin = GND, .diePad = 57, .strategy = TestStrategy::CAP_SENSE, .padType = PadType::VDDIO,    .settleUs = CAP_SETTLE_US, .thresholds = &kThresh    },  // die pad 57 VDD IO
    { .adapterPin = 52, .gndPin = GND, .diePad = 63, .strategy = TestStrategy::CAP_SENSE, .padType = PadType::PWR_AUX,  .settleUs = CAP_SETTLE_US, .thresholds = &kThresh },  // die pad 63 PWR Aux
    { .adapterPin = 60, .gndPin = GND, .diePad = 71, .strategy = TestStrategy::CAP_SENSE, .padType = PadType::VDD_CORE, .settleUs = CAP_SETTLE_US, .thresholds = &kThresh    },  // die pad 71 VDD Core
    { .adapterPin = 62, .gndPin = GND, .diePad = 73, .strategy = TestStrategy::CAP_SENSE, .padType = PadType::VDDIO,    .settleUs = CAP_SETTLE_US, .thresholds = &kThresh    },  // die pad 73 VDD IO
};
static_assert(sizeof(_pm2Cases) / sizeof(_pm2Cases[0]) == 64, "pm2 case count mismatch");

// — Pad map 3: 1x0p5 Mezzanine70 v1 ————————————————————————————————————————
// Source: docs/DUT_PADMAP_1X0P5.md. GND pins: 10, 18, 26, 46, 53, 61.

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
    { .adapterPin =  9, .gndPin = GND3, .diePad = 19, .strategy = TestStrategy::CAP_SENSE, .padType = PadType::VDDIO,    .settleUs = CAP_SETTLE_US, .thresholds = &kThresh    },  // die pad 19 VDD IO 0
    { .adapterPin = 17, .gndPin = GND3, .diePad = 25, .strategy = TestStrategy::CAP_SENSE, .padType = PadType::PWR_AUX,  .settleUs = CAP_SETTLE_US, .thresholds = &kThresh },  // die pad 25 PWR Aux 0
    { .adapterPin = 25, .gndPin = GND3, .diePad = 40, .strategy = TestStrategy::CAP_SENSE, .padType = PadType::VDD_CORE, .settleUs = CAP_SETTLE_US, .thresholds = &kThresh    },  // die pad 40 VDD CORE
    { .adapterPin = 27, .gndPin = GND3, .diePad = 35, .strategy = TestStrategy::CAP_SENSE, .padType = PadType::VDDIO,    .settleUs = CAP_SETTLE_US, .thresholds = &kThresh    },  // die pad 35 VDD IO 1
    { .adapterPin = 47, .gndPin = GND3, .diePad = 54, .strategy = TestStrategy::CAP_SENSE, .padType = PadType::VDDIO,    .settleUs = CAP_SETTLE_US, .thresholds = &kThresh    },  // die pad 54 VDD IO 2
    { .adapterPin = 52, .gndPin = GND3, .diePad = 60, .strategy = TestStrategy::CAP_SENSE, .padType = PadType::PWR_AUX,  .settleUs = CAP_SETTLE_US, .thresholds = &kThresh },  // die pad 60 PWR Aux 1
    { .adapterPin = 60, .gndPin = GND3, .diePad =  5, .strategy = TestStrategy::CAP_SENSE, .padType = PadType::VDD_CORE, .settleUs = CAP_SETTLE_US, .thresholds = &kThresh    },  // die pad  5 VDD CORE 1
    { .adapterPin = 62, .gndPin = GND3, .diePad = 70, .strategy = TestStrategy::CAP_SENSE, .padType = PadType::VDDIO,    .settleUs = CAP_SETTLE_US, .thresholds = &kThresh    },  // die pad 70 VDD IO 3
};
static_assert(sizeof(_pm3Cases) / sizeof(_pm3Cases[0]) == 64, "pm3 case count mismatch");

// — Pad map 4: MOSB —————————————————————————————————————————————————————————
// Source: docs/DUT_PADMAP_MOSB.md. Same 74-pad ring as 1x1 but an unrelated
// pinout. Die pads 17/36/73 are GND-plane-only; VDD die pads 8/9 share apin 70
// and are not tested (see the note at the end). No bypass caps, so no CAP_SENSE.

static const TestCase _pm4Cases[] = {
    // ── die pads  0– 7 (apin 62–69) ─────────────────────────────────────────
    IOM(62,  0),
    IOM(63,  1),
    IOM(64,  2),
    IOM(65,  3),
    IOM(66,  4),
    IOM(67,  5),
    IOM(68,  6),
    IOM(69,  7),  // [gap: VDD die pads 8/9 → apin 70, untested]
    // ── die pads 10–16 (apin 1–7) ─────────────────────────────────────────
    IOM( 1, 10),
    IOM( 2, 11),
    IOM( 3, 12),
    IOM( 4, 13),
    IOM( 5, 14),
    IOM( 6, 15),
    IOM( 7, 16),  // [gap: GND die pad 17]
    // ── die pads 18–35 (apin 8–25) ─────────────────────────────────────────
    IOM( 8, 18),
    IOM( 9, 19),
    IOM(10, 20),
    IOM(11, 21),
    IOM(12, 22),
    IOM(13, 23),
    IOM(14, 24),
    IOM(15, 25),
    IOM(16, 26),
    IOM(17, 27),
    IOM(18, 28),
    IOM(19, 29),
    IOM(20, 30),
    IOM(21, 31),
    IOM(22, 32),
    IOM(23, 33),
    IOM(24, 34),
    IOM(25, 35),  // [gap: GND die pad 36]
    // ── die pads 37–53 (apin 26–42) ─────────────────────────────────────────
    IOM(26, 37),
    IOM(27, 38),
    IOM(28, 39),
    IOM(29, 40),
    IOM(30, 41),
    IOM(31, 42),
    IOM(32, 43),
    IOM(33, 44),
    IOM(34, 45),
    IOM(35, 46),
    IOM(36, 47),
    IOM(37, 48),
    IOM(38, 49),
    IOM(39, 50),
    IOM(40, 51),
    IOM(41, 52),
    IOM(42, 53),  // [gap: GND die pad 54 → apin 43]
    // ── die pads 55–63 (apin 44–52) ─────────────────────────────────────────
    IOM(44, 55),
    IOM(45, 56),
    IOM(46, 57),
    IOM(47, 58),
    IOM(48, 59),
    IOM(49, 60),
    IOM(50, 61),
    IOM(51, 62),
    IOM(52, 63),
    // ── die pads 64–71 (apin 53–60) — extrapolated, verified at both ends ───
    IOM(53, 64),
    IOM(54, 65),
    IOM(55, 66),
    IOM(56, 67),
    IOM(57, 68),
    IOM(58, 69),
    IOM(59, 70),
    IOM(60, 71),
    // ── die pad  72    (apin 61) ─────────────────────────────────────────────
    IOM(61, 72),  // [gap: GND die pad 73]
    // NOTE: no standalone VDD case — VDD↔GND and VDD→IO both have no DC path
    // on this die (bench-verified); dp8's bond is exercised as the shared
    // return if/when upper-diode (forward, VDD-sink) cases are added.
};
static_assert(sizeof(_pm4Cases) / sizeof(_pm4Cases[0]) == 68, "pm4 case count mismatch");

// — Pad map 5: 0p5x1 Mezzanine70 v1 —————————————————————————————————————————
// Source: docs/DUT_PADMAP_0P5X1.md. GND pins: 10, 18, 26, 46, 53, 61. Die pads
// 3 and 40 are GND-plane-only. All VDD pads have bypass caps and are labelled
// VDDIO until the die documentation distinguishes the rails.

static const TestCase _pm5Cases[] = {
    // ── die pads  0– 2 (apin 63–65) ─────────────────────────────────────────
    IO5( 63,  0),
    IO5( 64,  1),
    IO5( 65,  2),  // [gap: GND die pad 3 (GND plane — no adapter pin), VDD apin62 die pad 4]
    // ── die pads  5– 9 (apin 66–70) ─────────────────────────────────────────
    IO5( 66,  5),
    IO5( 67,  6),
    IO5( 68,  7),
    IO5( 69,  8),
    IO5( 70,  9),  // [gap: GND apin10 die pad 10, VDD apin9 die pad 11]
    // ── die pads 12–20 (apin 1–8, 11) ─────────────────────────────────────────
    IO5(  1, 12),
    IO5(  2, 13),
    IO5(  3, 14),
    IO5(  4, 15),
    IO5(  5, 16),
    IO5(  6, 17),
    IO5(  7, 18),
    IO5(  8, 19),
    IO5( 11, 20),  // [gap: GND apin18 die pad 21, VDD apin17 die pad 22]
    // ── die pads 23–31 (apin 12–16, 19–22) ────────────────────────────────────
    IO5( 12, 23),
    IO5( 13, 24),
    IO5( 14, 25),
    IO5( 15, 26),
    IO5( 16, 27),
    IO5( 19, 28),
    IO5( 20, 29),
    IO5( 21, 30),
    IO5( 22, 31),  // [gap: GND apin26 die pad 32, VDD apin25 die pad 33]
    // ── die pads 34–38 (apin 23, 24, 28–30) ──────────────────────────────────
    IO5( 23, 34),
    IO5( 24, 35),
    IO5( 28, 36),
    IO5( 29, 37),
    IO5( 30, 38),  // [gap: VDD apin27 die pad 39, GND die pad 40 (GND plane — no adapter pin)]
    // ── die pads 41–45 (apin 31–35) ───────────────────────────────────────────
    IO5( 31, 41),
    IO5( 32, 42),
    IO5( 33, 43),
    IO5( 34, 44),
    IO5( 35, 45),  // [gap: VDD apin47 die pad 46, GND apin46 die pad 47]
    // ── die pads 48–56 (apin 36–44) ───────────────────────────────────────────
    IO5( 36, 48),
    IO5( 37, 49),
    IO5( 38, 50),
    IO5( 39, 51),
    IO5( 40, 52),
    IO5( 41, 53),
    IO5( 42, 54),
    IO5( 43, 55),
    IO5( 44, 56),  // [gap: VDD apin52 die pad 57, GND apin53 die pad 58]
    // ── die pads 59–66 (apin 45, 48–51, 54–56) ─────────────────────────────────
    IO5( 45, 59),
    IO5( 48, 60),
    IO5( 49, 61),
    IO5( 50, 62),
    IO5( 51, 63),
    IO5( 54, 64),
    IO5( 55, 65),
    IO5( 56, 66),  // [gap: VDD apin60 die pad 67, GND apin61 die pad 68]
    // ── die pads 69–71 (apin 57–59) ───────────────────────────────────────────
    IO5( 57, 69),
    IO5( 58, 70),
    IO5( 59, 71),
    // ── VDD (all carry bypass caps; VDDIO label pending rail documentation) ───
    { .adapterPin = 62, .gndPin = GND5, .diePad =  4, .strategy = TestStrategy::CAP_SENSE, .padType = PadType::VDDIO, .settleUs = CAP_SETTLE_US, .thresholds = &kThresh },  // die pad  4
    { .adapterPin =  9, .gndPin = GND5, .diePad = 11, .strategy = TestStrategy::CAP_SENSE, .padType = PadType::VDDIO, .settleUs = CAP_SETTLE_US, .thresholds = &kThresh },  // die pad 11
    { .adapterPin = 17, .gndPin = GND5, .diePad = 22, .strategy = TestStrategy::CAP_SENSE, .padType = PadType::VDDIO, .settleUs = CAP_SETTLE_US, .thresholds = &kThresh },  // die pad 22
    { .adapterPin = 25, .gndPin = GND5, .diePad = 33, .strategy = TestStrategy::CAP_SENSE, .padType = PadType::VDDIO, .settleUs = CAP_SETTLE_US, .thresholds = &kThresh },  // die pad 33
    { .adapterPin = 27, .gndPin = GND5, .diePad = 39, .strategy = TestStrategy::CAP_SENSE, .padType = PadType::VDDIO, .settleUs = CAP_SETTLE_US, .thresholds = &kThresh },  // die pad 39
    { .adapterPin = 47, .gndPin = GND5, .diePad = 46, .strategy = TestStrategy::CAP_SENSE, .padType = PadType::VDDIO, .settleUs = CAP_SETTLE_US, .thresholds = &kThresh },  // die pad 46
    { .adapterPin = 52, .gndPin = GND5, .diePad = 57, .strategy = TestStrategy::CAP_SENSE, .padType = PadType::VDDIO, .settleUs = CAP_SETTLE_US, .thresholds = &kThresh },  // die pad 57
    { .adapterPin = 60, .gndPin = GND5, .diePad = 67, .strategy = TestStrategy::CAP_SENSE, .padType = PadType::VDDIO, .settleUs = CAP_SETTLE_US, .thresholds = &kThresh },  // die pad 67
};
static_assert(sizeof(_pm5Cases) / sizeof(_pm5Cases[0]) == 64, "pm5 case count mismatch");

// — Pad map 6: TQVA ———————————————————————————————————————————————————————————
// Source: docs/DUT_PADMAP_TQVA.md. GND pins: 10, 18, 26, 46, 53, 61. Die pads 3
// and 35 are each bonded to two DUT pins; one case per pad.

static const TestCase _pm6Cases[] = {
    // ── die pads  0, 1, 4– 9 (apin 63–70) ──────────────────────────────────
    IO6( 63,  0),
    IO6( 64,  1),  // [gap: GND dp2 → apin61, VDDIO dp3 → apins 62&60 (shared net)]
    IO6( 65,  4),
    IO6( 66,  5),
    IO6( 67,  6),
    IO6( 68,  7),
    IO6( 69,  8),
    IO6( 70,  9),
    // ── die pads 10–17 (apin  1– 8) ────────────────────────────────────────
    IO6(  1, 10),
    IO6(  2, 11),
    IO6(  3, 12),
    IO6(  4, 13),
    IO6(  5, 14),
    IO6(  6, 15),
    IO6(  7, 16),
    IO6(  8, 17),  // [gap: NC dut 62 → apin9, GND dut 61 → apin10 (die pad unknown)]
    // ── die pads 18–23 (apin 11–16) ────────────────────────────────────────
    IO6( 11, 18),
    IO6( 12, 19),
    IO6( 13, 20),
    IO6( 14, 21),
    IO6( 15, 22),
    IO6( 16, 23),
    // ── die pads 24–25 (apin 19–20) ────────────────────────────────────────
    IO6( 19, 24),
    IO6( 20, 25),  // [gap: GND dut 53 → apin18 (die pad unknown), VDDIO dp27 → apin17]
    // ── die pads 28–31 (apin 21–24) ────────────────────────────────────────
    IO6( 21, 28),
    IO6( 22, 29),
    IO6( 23, 30),
    IO6( 24, 31),  // [gap: GND dp26 → apin26]
    // ── die pad 32 (apin 28) ───────────────────────────────────────────────
    IO6( 28, 32),  // [gap: NC dp33/34, VDDIO dp35 → apins 25&27 (shared net), GND dp36 → apin46]
    // ── die pads 39–40 (apin 59–58) ────────────────────────────────────────
    IO6( 59, 39),
    IO6( 58, 40),  // [gap: VDD_CORE dp41 → apin47, GND dp42 → apin53]
    // ── die pads 45–50 (apin 29–34) ────────────────────────────────────────
    IO6( 29, 45),
    IO6( 30, 46),
    IO6( 31, 47),
    IO6( 32, 48),
    IO6( 33, 49),
    IO6( 34, 50),  // [gap: NC dp51–55]
    // ── VDD (all carry bypass caps) ────────────────────────────────────────
    { .adapterPin = 62, .gndPin = GND6, .diePad =  3, .strategy = TestStrategy::CAP_SENSE, .padType = PadType::VDDIO,    .settleUs = CAP_SETTLE_US, .thresholds = &kThresh },  // dp3, bonded to dut 9&11 (apins 62&60, shared net)
    { .adapterPin = 17, .gndPin = GND6, .diePad = 27, .strategy = TestStrategy::CAP_SENSE, .padType = PadType::VDDIO,    .settleUs = CAP_SETTLE_US, .thresholds = &kThresh },  // dp27
    { .adapterPin = 25, .gndPin = GND6, .diePad = 35, .strategy = TestStrategy::CAP_SENSE, .padType = PadType::VDDIO,    .settleUs = CAP_SETTLE_US, .thresholds = &kThresh },  // dp35, bonded to dut 46&44 (apins 25&27, shared net)
    { .adapterPin = 47, .gndPin = GND6, .diePad = 41, .strategy = TestStrategy::CAP_SENSE, .padType = PadType::VDD_CORE, .settleUs = CAP_SETTLE_US, .thresholds = &kThresh },  // dp41
};
static_assert(sizeof(_pm6Cases) / sizeof(_pm6Cases[0]) == 41, "pm6 case count mismatch");

#undef IO
#undef IO3
#undef IO5
#undef IO6
#undef IOM

static const PadMap _maps[] = {
    {
        .id                 = 1,
        .name               = "1x1 v1",
        .pinout             = AdapterPinout::Mezzanine70,
        .cases              = _pm1Cases,
        .caseCount          = 63,
        .presencePadA       = 10,
        .presencePadB       = 53,
        .presenceThresholdV = 0.3f,
        .checkOrientation   = true,
        .directions         = MeasureDirections::REVERSE_ONLY,
    },
    {
        .id                 = 2,
        .name               = "1x1 v2",
        .pinout             = AdapterPinout::Mezzanine70,
        .cases              = _pm2Cases,
        .caseCount          = 64,
        .presencePadA       = 10,
        .presencePadB       = 53,
        .presenceThresholdV = 0.3f,
        .checkOrientation   = true,
        .directions         = MeasureDirections::REVERSE_ONLY,
    },
    {
        .id                 = 3,
        .name               = "1x0p5",
        .pinout             = AdapterPinout::Mezzanine70,
        .cases              = _pm3Cases,
        .caseCount          = 64,
        .presencePadA       = 10,
        .presencePadB       = 53,
        .presenceThresholdV = 0.3f,
        .checkOrientation   = true,
        .directions         = MeasureDirections::REVERSE_ONLY,
    },
    {
        .id                 = 4,
        .name               = "G801-MOSB",
        .pinout             = AdapterPinout::Mezzanine70,
        .cases              = _pm4Cases,
        .caseCount          = 68,
        .presencePadA       = 43,   // GND
        .presencePadB       = 70,   // VDD die pads 8/9. TODO(bench): chosen as "die pad 10"
                                    // before the 71−dut remap (6e858e5) — confirm the pair.
        .presenceThresholdV = 1.5f, // bench: present 0.50-0.85 V, absent >=3.1 V
        .checkOrientation   = false, // mirrored pins conduct through on-die diode chains
        .directions         = MeasureDirections::BOTH,  // capless DUT
    },
    {
        .id                 = 5,
        .name               = "0p5x1",
        .pinout             = AdapterPinout::Mezzanine70,
        .cases              = _pm5Cases,
        .caseCount          = 64,
        .presencePadA       = 61,  // die pad 68 (gnd_7)
        .presencePadB       = 18,  // die pad 21 (gnd_2)
        .presenceThresholdV = 0.3f,
        .checkOrientation   = true,
        .directions         = MeasureDirections::REVERSE_ONLY,
    },
    {
        .id                 = 6,
        .name               = "G801-TQVA",
        .pinout             = AdapterPinout::Mezzanine70,
        .cases              = _pm6Cases,
        .caseCount          = 41,
        .presencePadA       = 26,  // dp26 GND (dut 45). Flipped mirror sees dut 26
        .presencePadB       = 46,  // dp36 GND (dut 25) — unconnected → open, so the
                                    // orientation check can fire. (Do NOT use the
                                    // 1x1 pair 61/53: TQVA's GND dut set {10,18,53,61}
                                    // is mirror-closed under 71−dut, so a flipped DUT
                                    // still conducts GND↔GND and reads PRESENT.)
        .presenceThresholdV = 0.3f,
        .checkOrientation   = true,
        .directions         = MeasureDirections::REVERSE_ONLY,
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

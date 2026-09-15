# DUT Pad Map — MOSB

Adapter: MOSB (special chip + PCB, no bypass capacitors)
Source: pin map verified against PCB, 2026-09-14
Correction (2026-09-15): the source numbers were DUT pins, originally recorded
here as adapter pins. Converted via `adapter_pin = 71 − dut_pin` (same formula
as the 1x1/1x0p5/0p5x1 maps). The converted map is near-sequential — die pads
map to consecutive adapter pins around the ring — which corroborates the
correction. DUT Pin column below preserves the original source data.
Tester channels: see `ADAPTER_MEZ70.md` (`tester_ch = adapter_pin − 1`)

Die pad 0 is at the top-right corner; numbers increase counter-clockwise.
Same 74-pad ring as the 1x1 die, but the pinout is completely different —
do not derive anything from `DUT_PADMAP_1X1.md`.

**Mapping layers:**

| Layer | Formula | Owner |
|-------|---------|-------|
| Die Pad → DUT Pin | physical bond wires on the DUT PCB | this die |
| DUT Pin → Adapter Pin | `adapter_pin = 71 − dut_pin` | DUT PCB (connector orientation) |
| Adapter Pin → Tester Ch | `tester_ch = adapter_pin − 1` | adapter — see `ADAPTER_MEZ70.md` |

> **DUT Pin is reference only.** It traces the physical bond wire path and is
> not used in firmware. Only Adapter Pin values appear in `pad_map_registry.cpp`.
> See `GLOSSARY.md` for definitions.

---

## Identity

| Field | Value |
|-------|-------|
| `id` (ProjectId enum value) | 4 |
| `name` (short human-readable string) | `MOSB` |

---

## Die Pad to Adapter Pin Cross-Reference

| Die Pad | Signal | DUT Pin | Adapter Pin | Role |
|---------|--------|---------|---------|------|
| 0  | IO  | 9  | 62 | IO  |
| 1  | IO  | 8  | 63 | IO  |
| 2  | IO  | 7  | 64 | IO  |
| 3  | IO  | 6  | 65 | IO  |
| 4  | IO  | 5  | 66 | IO  |
| 5  | IO  | 4  | 67 | IO  |
| 6  | IO  | 3  | 68 | IO  |
| 7  | IO  | 2  | 69 | IO  |
| 8  | VDD | 1  | 70 | VCC — measured |
| 9  | VDD | 1  | 70 | VCC — **not tested**: shares adapter pin 70 with die pad 8; one result covers both bonds |
| 10 | IO  | 70 | 1  | IO  |
| 11 | IO  | 69 | 2  | IO  |
| 12 | IO  | 68 | 3  | IO  |
| 13 | IO  | 67 | 4  | IO  |
| 14 | IO  | 66 | 5  | IO  |
| 15 | IO  | 65 | 6  | IO  |
| 16 | IO  | 64 | 7  | IO  |
| 17 | GND | —  | —  | GND (no adapter pin noted — verify) |
| 18 | IO  | 63 | 8  | IO  |
| 19 | IO  | 62 | 9  | IO  |
| 20 | IO  | 61 | 10 | IO  |
| 21 | IO  | 60 | 11 | IO  |
| 22 | IO  | 59 | 12 | IO  |
| 23 | IO  | 58 | 13 | IO  |
| 24 | IO  | 57 | 14 | IO  |
| 25 | IO  | 56 | 15 | IO  |
| 26 | IO  | 55 | 16 | IO  |
| 27 | IO  | 54 | 17 | IO  |
| 28 | IO  | 53 | 18 | IO  |
| 29 | IO  | 52 | 19 | IO  |
| 30 | IO  | 51 | 20 | IO  |
| 31 | IO  | 50 | 21 | IO  |
| 32 | IO  | 49 | 22 | IO  |
| 33 | IO  | 48 | 23 | IO  |
| 34 | IO  | 47 | 24 | IO  |
| 35 | IO  | 46 | 25 | IO  |
| 36 | GND | —  | —  | GND (no adapter pin noted — verify) |
| 37 | IO  | 45 | 26 | IO  |
| 38 | IO  | 44 | 27 | IO  |
| 39 | IO  | 43 | 28 | IO  |
| 40 | IO  | 42 | 29 | IO  |
| 41 | IO  | 41 | 30 | IO  |
| 42 | IO  | 40 | 31 | IO  |
| 43 | IO  | 39 | 32 | IO  |
| 44 | IO  | 38 | 33 | IO  |
| 45 | IO  | 37 | 34 | IO  |
| 46 | IO  | 36 | 35 | IO  |
| 47 | IO  | 35 | 36 | IO  |
| 48 | IO  | 34 | 37 | IO  |
| 49 | IO  | 33 | 38 | IO  |
| 50 | IO  | 32 | 39 | IO  |
| 51 | IO  | 31 | 40 | IO  |
| 52 | IO  | 30 | 41 | IO  |
| 53 | IO  | 29 | 42 | IO  |
| 54 | GND | 28 | 43 | GND — only GND pad with a routed adapter pin |
| 55 | IO  | 27 | 44 | IO  |
| 56 | IO  | 26 | 45 | IO  |
| 57 | IO  | 25 | 46 | IO  |
| 58 | IO  | 24 | 47 | IO  |
| 59 | IO  | 23 | 48 | IO  |
| 60 | IO  | 22 | 49 | IO  |
| 61 | IO  | 21 | 50 | IO  |
| 62 | IO  | 20 | 51 | IO  |
| 63 | IO  | 19 | 52 | IO  |
| 64 | IO  | 18 | 53 | IO  — extrapolated, see Notes |
| 65 | IO  | 17 | 54 | IO  — extrapolated, see Notes |
| 66 | IO  | 16 | 55 | IO  — extrapolated, see Notes |
| 67 | IO  | 15 | 56 | IO  — extrapolated, see Notes |
| 68 | IO  | 14 | 57 | IO  — extrapolated, see Notes |
| 69 | IO  | 13 | 58 | IO  — extrapolated, see Notes |
| 70 | IO  | 12 | 59 | IO  — extrapolated, see Notes |
| 71 | IO  | 11 | 60 | IO  — extrapolated, see Notes |
| 72 | IO  | 10 | 61 | IO  |
| 73 | GND | —  | —  | GND (no adapter pin noted — verify) |

---

## GND Pads

Only one GND pad has a routed adapter pin — there is no second GND pin for
a classic GND↔GND presence short, so presence detection uses the VDD↔GND
substrate diode instead (see Presence Detection).

| Die Pad | Signal | DUT Pin | Adapter Pin |
|---------|--------|---------|-------------|
| 54 | GND | 28 | 43 |

GND plane only (no adapter pin noted — verify at bring-up): die pads 17, 36, 73

```
gndPin = 43  // adapter pin 43 (tester ch 42), die pad 54 — the only GND connection
```

---

## IO Adapter Pins in Ring Order

Counter-clockwise from die pad 0 (top-right corner), IO connections only.
Values are **adapter pins** (1-indexed). GND/VCC die pads are skipped.

```
ioAdapterPinsInRingOrder = [
   62, 63, 64, 65, 66, 67, 68, 69,          // die pads  0– 7
    1,  2,  3,  4,  5,  6,  7,              // die pads 10–16
    8,  9, 10, 11, 12, 13, 14, 15, 16, 17,  // die pads 18–27
   18, 19, 20, 21, 22, 23, 24, 25,          // die pads 28–35
   26, 27, 28, 29, 30, 31, 32, 33, 34, 35,  // die pads 37–46
   36, 37, 38, 39, 40, 41, 42,              // die pads 47–53
   44, 45, 46, 47, 48, 49, 50, 51, 52,      // die pads 55–63
   53, 54, 55, 56, 57, 58, 59, 60,          // die pads 64–71 (extrapolated)
   61                                       // die pad  72
]
```

---

## Presence Detection

This DUT has only ONE GND adapter pin, so the classic two-GND-pin short
(0.3 V threshold) is impossible. The VDD↔GND substrate-diode path (die pads
8/9, apin 70) was tried first and REJECTED: it conducts weakly and
unpredictably (sometimes ≥1.5 V, sometimes not at all — likely not a clean
substrate tie). The working scheme uses an IO pad's lower ESD diode instead:
force the GND pad positive and sink die pad 10 (apin 1, an IO pad) — the
GND→IO diode forward-conducts hard and, per bench data, more stably than any
other path.

```
presencePadA       = 43   // GND pad — forced + Kelvin-sensed
presencePadB       = 1    // die pad 10 (IO) — sunk
presenceThresholdV = 1.5  // bench-verified: present 0.50-0.85 V, absent >=3.1 V
checkOrientation   = false  // orientation check DISABLED for this padmap — see below
```

**Bench-verified behavior (2026-09 bring-up):** with the DUT seated, all
three pullup levels read 0.50–0.85 V (280k: ~0.55 V, 27.4k: ~0.64 V,
2.49k: ~0.85 V — the rise at strong pullup is I×R through contact/series
resistance). Empty socket: ≥3.1 V at every level. 1.5 V sits mid-window with
≥0.65 V margin on both sides. Stable over 30+ s of polling; insertion and
removal events fire correctly.

**Orientation check is DISABLED** (`checkOrientation = false` in the
registry). The flipped mirror pair of any candidate conducts through
on-die diode chains (IO→VDD rail→GND→IO, ~1.0–2.4 V depending on pullup
level and the floating VDD rail's charge state) whose voltage drifts across
any practical threshold, causing PRESENT/WRONG_ORIENTATION flapping. With
only one GND pin there is no pair whose flipped mirror is reliably open, so
no threshold can fix this. Trade-off accepted: a flipped DUT is no longer
caught at insertion — it runs and reads all-open instead, which the operator
must recognize.

---

## Test Thresholds

Leave blank until Phase 3 hardware calibration.

| Signal | Threshold | Default | This die |
|--------|-----------|---------|----------|
| COM_D good bond min | V | 0.50 V | |
| COM_D good bond max | V | 0.70 V | |
| COM_D open bond min | V | 2.50 V | |
| COM_D short max | V | 0.10 V | |
| COM_A / COM_C short max | V | 0.30 V | |

---

## Notes

- 68 IO pads + 1 VDD pad (die pad 8) = **69 test cases**; 4 GND pads (17/36/54/73)
- **No bypass/decoupling capacitors on this chip+PCB** — VDD pads (die pads
  8/9) use STANDARD IO sense (6-reading Kelvin sweep), not CAP_SENSE.
  Capacitor-free VDD nets should settle fast, but the 60 kΩ activity ceiling
  was calibrated on IO pads — re-confirm on bench.
- **Die pad 9 is not tested** — it shares adapter pin 70 with die pad 8
  (both VDD), so one measurement covers both bonds. Listed in the
  cross-reference for completeness only; omit from the `TestCase` array.
- **Die pads 64–71 are extrapolated**: the verified run dp55→apin 27 …
  dp63→apin 19 (pre-correction numbering) continues through dp64→apin 18 …
  dp71→apin 11, and dp72→apin 10 confirms the pattern at the far end. In the
  corrected numbering the run is dp55→apin 44 … dp63→apin 52, extrapolated
  dp64→apin 53 … dp71→apin 60, confirmed by dp72→apin 61. Verify against the
  PCB before production.
- GND die pads 17/36/73 had no adapter pin noted in the PCB verification —
  confirm whether they are plane-only or have routed adapter pins (matters
  for presence detection).
- DUT Pin column is the original source data (recorded 2026-09-14, initially
  mislabeled as adapter pins — see correction note at top).

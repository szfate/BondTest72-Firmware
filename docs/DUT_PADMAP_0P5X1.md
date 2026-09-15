# DUT Pad Map — 0p5x1

Adapter: 0p5x1 (COB die on Mezzanine70 connector)
Source: user-provided die-pad→DUT-pin cross-reference (2026-09-14); `...`
gaps in the source filled linearly — the completed map uses DUT pins 1–70
exactly once (self-consistency check).
Correction (2026-09-15): the source numbers were DUT pins, originally recorded
here as adapter pins. Converted via `adapter_pin = 71 − dut_pin` (same formula
as the 1x1/1x0p5 maps). DUT Pin column below preserves the original source
data.
Tester channels: see `ADAPTER_MEZ70.md` (`tester_ch = adapter_pin − 1`)

Die pad 0 is at the top-right corner; numbers increase counter-clockwise.

**Mapping layers:**

| Layer | Formula | Owner |
|-------|---------|-------|
| Die Pad → DUT Pin | physical bond wires on the DUT PCB | this die |
| DUT Pin → Adapter Pin | `adapter_pin = 71 − dut_pin` | DUT PCB (connector orientation) |
| Adapter Pin → Tester Ch | `tester_ch = adapter_pin − 1` | adapter — see `ADAPTER_MEZ70.md` |

**Signal names are placeholders.** Unlike 1x0p5 (which has the
`clk/rst_n/bidir_N/an_N/in_N` names from the die docs), the 0p5x1 signal
names were not in the source mapping. IO pads are labelled `io_0`…`io_55` in
ring order, VDD pads `vdd_0`…`vdd_7`, GND pads `gnd_0`…`gnd_7` — replace with
the real names when the die documentation is available.

---

## Identity

| Field | Value |
|-------|-------|
| `id` (pad map registry) | 5 |
| `name` (short human-readable string) | `0p5x1` |

---

## Die Pad to Adapter Pin Cross-Reference

| Die Pad | Signal  | DUT Pin | Adapter Pin | Role |
|---------|---------|---------|-------------|------|
| 0  | io_0  | 8  | 63 | IO  |
| 1  | io_1  | 7  | 64 | IO  |
| 2  | io_2  | 6  | 65 | IO  |
| 3  | gnd_0 | —  | —  | GND (DUT GND plane — no DUT pin) |
| 4  | vdd_0 | 9  | 62 | VCC |
| 5  | io_3  | 5  | 66 | IO  |
| 6  | io_4  | 4  | 67 | IO  |
| 7  | io_5  | 3  | 68 | IO  |
| 8  | io_6  | 2  | 69 | IO  |
| 9  | io_7  | 1  | 70 | IO  |
| 10 | gnd_1 | 61 | 10 | GND |
| 11 | vdd_1 | 62 | 9  | VCC |
| 12 | io_8  | 70 | 1  | IO  |
| 13 | io_9  | 69 | 2  | IO  |
| 14 | io_10 | 68 | 3  | IO  |
| 15 | io_11 | 67 | 4  | IO  |
| 16 | io_12 | 66 | 5  | IO  |
| 17 | io_13 | 65 | 6  | IO  |
| 18 | io_14 | 64 | 7  | IO  |
| 19 | io_15 | 63 | 8  | IO  |
| 20 | io_16 | 60 | 11 | IO  |
| 21 | gnd_2 | 53 | 18 | GND |
| 22 | vdd_2 | 54 | 17 | VCC |
| 23 | io_17 | 59 | 12 | IO  |
| 24 | io_18 | 58 | 13 | IO  |
| 25 | io_19 | 57 | 14 | IO  |
| 26 | io_20 | 56 | 15 | IO  |
| 27 | io_21 | 55 | 16 | IO  |
| 28 | io_22 | 52 | 19 | IO  |
| 29 | io_23 | 51 | 20 | IO  |
| 30 | io_24 | 50 | 21 | IO  |
| 31 | io_25 | 49 | 22 | IO  |
| 32 | gnd_3 | 45 | 26 | GND |
| 33 | vdd_3 | 46 | 25 | VCC |
| 34 | io_26 | 48 | 23 | IO  |
| 35 | io_27 | 47 | 24 | IO  |
| 36 | io_28 | 43 | 28 | IO  |
| 37 | io_29 | 42 | 29 | IO  |
| 38 | io_30 | 41 | 30 | IO  |
| 39 | vdd_4 | 44 | 27 | VCC |
| 40 | gnd_4 | —  | —  | GND (DUT GND plane — no DUT pin) |
| 41 | io_31 | 40 | 31 | IO  |
| 42 | io_32 | 39 | 32 | IO  |
| 43 | io_33 | 38 | 33 | IO  |
| 44 | io_34 | 37 | 34 | IO  |
| 45 | io_35 | 36 | 35 | IO  |
| 46 | vdd_5 | 24 | 47 | VCC |
| 47 | gnd_5 | 25 | 46 | GND |
| 48 | io_36 | 35 | 36 | IO  |
| 49 | io_37 | 34 | 37 | IO  |
| 50 | io_38 | 33 | 38 | IO  |
| 51 | io_39 | 32 | 39 | IO  |
| 52 | io_40 | 31 | 40 | IO  |
| 53 | io_41 | 30 | 41 | IO  |
| 54 | io_42 | 29 | 42 | IO  |
| 55 | io_43 | 28 | 43 | IO  |
| 56 | io_44 | 27 | 44 | IO  |
| 57 | vdd_6 | 19 | 52 | VCC |
| 58 | gnd_6 | 18 | 53 | GND |
| 59 | io_45 | 26 | 45 | IO  |
| 60 | io_46 | 23 | 48 | IO  |
| 61 | io_47 | 22 | 49 | IO  |
| 62 | io_48 | 21 | 50 | IO  |
| 63 | io_49 | 20 | 51 | IO  |
| 64 | io_50 | 17 | 54 | IO  |
| 65 | io_51 | 16 | 55 | IO  |
| 66 | io_52 | 15 | 56 | IO  |
| 67 | vdd_7 | 11 | 60 | VCC |
| 68 | gnd_7 | 10 | 61 | GND |
| 69 | io_53 | 14 | 57 | IO  |
| 70 | io_54 | 13 | 58 | IO  |
| 71 | io_55 | 12 | 59 | IO  |

---

## GND Pads

Six GND pads have individual adapter connector pins; two are bonded to the DUT GND plane only.

| Die Pad | Signal | DUT Pin | Adapter Pin |
|---------|--------|---------|-------------|
| 10 | gnd_1 | 61 | 10 |
| 21 | gnd_2 | 53 | 18 |
| 32 | gnd_3 | 45 | 26 |
| 47 | gnd_5 | 25 | 46 |
| 58 | gnd_6 | 18 | 53 |
| 68 | gnd_7 | 10 | 61 |

GND plane only (no DUT pin, no adapter pin): die pads 3, 40

```
gndPin = 26  // adapter pin 26 (tester ch 25), die pad 32 — near geometric centre of bond ring
```

---

## IO Adapter Pins in Ring Order

Counter-clockwise from die pad 0 (top-right corner), IO connections only.
Values are **adapter pins** (1-indexed). GND/VCC die pads are skipped.

```
ioAdapterPinsInRingOrder = [
   63, 64, 65,                               // die pads  0– 2: io_0–2
   66, 67, 68, 69, 70,                       // die pads  5– 9: io_3–7
    1,  2,  3,  4,  5,  6,  7,  8, 11,       // die pads 12–20: io_8–16
   12, 13, 14, 15, 16, 19, 20, 21, 22,       // die pads 23–31: io_17–25
   23, 24, 28, 29, 30,                       // die pads 34–38: io_26–30
   31, 32, 33, 34, 35,                       // die pads 41–45: io_31–35
   36, 37, 38, 39, 40, 41, 42, 43, 44,       // die pads 48–56: io_36–44
   45, 48, 49, 50, 51, 54, 55, 56,           // die pads 59–66: io_45–52
   57, 58, 59                                // die pads 69–71: io_53–55
]
```

---

## Presence Detection

| Die Pad | Signal | DUT Pin | Adapter Pin |
|---------|--------|---------|-------------|
| 68 | gnd_7 | 10 | 61 |
| 21 | gnd_2 | 53 | 18 |

```
presenceThresholdV = 0.3  // reuses the 1x0p5 default — calibrate in Phase 3 if needed
```

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

- Die dimensions: 1.94 mm × 5.12 mm (N/S width × E/W height)
- 56 IO pads (`io_0`–`io_55`, real names TBD from die docs) + 8 VCC + 8 GND = 72 pads
- All 8 VCC pads carry bypass caps → CAP_SENSE strategy.
  `MeasureDirections::BOTH` enabled 2026-09-15 — forward drive on bypass-capped
  VDD pads carries the CH446X latch-up trigger risk documented in pad_map.h;
  watch for it on the bench (REVERSE_ONLY was the conservative default)
- All 8 VCC cases are currently labelled `VDDIO`; relabel to VDD_CORE / PWR_AUX per
  the die docs when the rail breakdown is known
- 6 GND pads with individual adapter pins; die pads 3 and 40 are bonded to the
  DUT GND plane only
- Mirror counterpart of the 1x0p5 map (pad map 3): same 72-pad ring shape
  (8 GND+VDD pairs + 56 IO), different pinout

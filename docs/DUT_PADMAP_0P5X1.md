# DUT Pad Map — 0p5x1

Adapter: 0p5x1 (COB die on Mezzanine70 connector)
Source: user-provided die-pad→adapter-pin cross-reference (2026-09-14); `...`
gaps in the source filled linearly — the completed map uses adapter pins 1–70
exactly once (self-consistency check).
Tester channels: see `ADAPTER_MEZ70.md` (`tester_ch = adapter_pin − 1`)

Die pad 0 is at the top-right corner; numbers increase counter-clockwise.

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

| Die Pad | Signal  | Adapter Pin | Role |
|---------|---------|-------------|------|
| 0  | io_0  | 8  | IO  |
| 1  | io_1  | 7  | IO  |
| 2  | io_2  | 6  | IO  |
| 3  | gnd_0 | —  | GND (DUT GND plane — no adapter pin) |
| 4  | vdd_0 | 9  | VCC |
| 5  | io_3  | 5  | IO  |
| 6  | io_4  | 4  | IO  |
| 7  | io_5  | 3  | IO  |
| 8  | io_6  | 2  | IO  |
| 9  | io_7  | 1  | IO  |
| 10 | gnd_1 | 61 | GND |
| 11 | vdd_1 | 62 | VCC |
| 12 | io_8  | 70 | IO  |
| 13 | io_9  | 69 | IO  |
| 14 | io_10 | 68 | IO  |
| 15 | io_11 | 67 | IO  |
| 16 | io_12 | 66 | IO  |
| 17 | io_13 | 65 | IO  |
| 18 | io_14 | 64 | IO  |
| 19 | io_15 | 63 | IO  |
| 20 | io_16 | 60 | IO  |
| 21 | gnd_2 | 53 | GND |
| 22 | vdd_2 | 54 | VCC |
| 23 | io_17 | 59 | IO  |
| 24 | io_18 | 58 | IO  |
| 25 | io_19 | 57 | IO  |
| 26 | io_20 | 56 | IO  |
| 27 | io_21 | 55 | IO  |
| 28 | io_22 | 52 | IO  |
| 29 | io_23 | 51 | IO  |
| 30 | io_24 | 50 | IO  |
| 31 | io_25 | 49 | IO  |
| 32 | gnd_3 | 45 | GND |
| 33 | vdd_3 | 46 | VCC |
| 34 | io_26 | 48 | IO  |
| 35 | io_27 | 47 | IO  |
| 36 | io_28 | 43 | IO  |
| 37 | io_29 | 42 | IO  |
| 38 | io_30 | 41 | IO  |
| 39 | vdd_4 | 44 | VCC |
| 40 | gnd_4 | —  | GND (DUT GND plane — no adapter pin) |
| 41 | io_31 | 40 | IO  |
| 42 | io_32 | 39 | IO  |
| 43 | io_33 | 38 | IO  |
| 44 | io_34 | 37 | IO  |
| 45 | io_35 | 36 | IO  |
| 46 | vdd_5 | 24 | VCC |
| 47 | gnd_5 | 25 | GND |
| 48 | io_36 | 35 | IO  |
| 49 | io_37 | 34 | IO  |
| 50 | io_38 | 33 | IO  |
| 51 | io_39 | 32 | IO  |
| 52 | io_40 | 31 | IO  |
| 53 | io_41 | 30 | IO  |
| 54 | io_42 | 29 | IO  |
| 55 | io_43 | 28 | IO  |
| 56 | io_44 | 27 | IO  |
| 57 | vdd_6 | 19 | VCC |
| 58 | gnd_6 | 18 | GND |
| 59 | io_45 | 26 | IO  |
| 60 | io_46 | 23 | IO  |
| 61 | io_47 | 22 | IO  |
| 62 | io_48 | 21 | IO  |
| 63 | io_49 | 20 | IO  |
| 64 | io_50 | 17 | IO  |
| 65 | io_51 | 16 | IO  |
| 66 | io_52 | 15 | IO  |
| 67 | vdd_7 | 11 | VCC |
| 68 | gnd_7 | 10 | GND |
| 69 | io_53 | 14 | IO  |
| 70 | io_54 | 13 | IO  |
| 71 | io_55 | 12 | IO  |

---

## GND Pads

Six GND pads have individual adapter connector pins; two are bonded to the DUT GND plane only.

| Die Pad | Signal | Adapter Pin |
|---------|--------|-------------|
| 10 | gnd_1 | 61 |
| 21 | gnd_2 | 53 |
| 32 | gnd_3 | 45 |
| 47 | gnd_5 | 25 |
| 58 | gnd_6 | 18 |
| 68 | gnd_7 | 10 |

GND plane only (no adapter pin): die pads 3, 40

```
gndPin = 45  // adapter pin 45 (tester ch 44), die pad 32 — near geometric centre of bond ring
```

---

## IO Adapter Pins in Ring Order

Counter-clockwise from die pad 0 (top-right corner), IO connections only.
Values are **adapter pins** (1-indexed). GND/VCC die pads are skipped.

```
ioAdapterPinsInRingOrder = [
   8,  7,  6,  5,  4,  3,  2,  1,          // die pads  0– 9: io_0–7
  70, 69, 68, 67, 66, 65, 64, 63, 60,       // die pads 12–20: io_8–16
  59, 58, 57, 56, 55, 52, 51, 50, 49,       // die pads 23–31: io_17–25
  48, 47, 43, 42, 41,                       // die pads 34–38: io_26–30
  40, 39, 38, 37, 36,                       // die pads 41–45: io_31–35
  35, 34, 33, 32, 31, 30, 29, 28, 27,       // die pads 48–56: io_36–44
  26, 23, 22, 21, 20, 17, 16, 15,           // die pads 59–66: io_45–52
  14, 13, 12                                // die pads 69–71: io_53–55
]
```

---

## Presence Detection

| Die Pad | Signal | Adapter Pin |
|---------|--------|-------------|
| 68 | gnd_7 | 10 |
| 21 | gnd_2 | 53 |

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
- All 8 VCC pads carry bypass caps → CAP_SENSE strategy, `MeasureDirections::REVERSE_ONLY`
  (BOTH is unsafe with bypass caps — CH446X latch-up trigger sequence)
- All 8 VCC cases are currently labelled `VDDIO`; relabel to VDD_CORE / PWR_AUX per
  the die docs when the rail breakdown is known
- 6 GND pads with individual adapter pins; die pads 3 and 40 are bonded to the
  DUT GND plane only
- Mirror counterpart of the 1x0p5 map (pad map 3): same 72-pad ring shape
  (8 GND+VDD pairs + 56 IO), different pinout

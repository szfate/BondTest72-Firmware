# DUT Pad Map — 1x0p5

Adapter: 1x0p5 (COB die on 0.5 mm pitch mezzanine adapter)
Source: `2026-05-18 Working out pin mapping.xlsx` sheet `1x0p5`
Tester channels: see `ADAPTER_MEZ70.md` (`tester_ch = adapter_pin − 1`)

Die pad 0 is at the top-right corner; numbers increase counter-clockwise.

**Mapping layers:**

| Layer | Formula | Owner |
|-------|---------|-------|
| Die Pad → DUT Pin | physical bond wires on the DUT PCB | this die |
| DUT Pin → Adapter Pin | `adapter_pin = 71 − dut_pin` | DUT PCB (connector orientation) |
| Adapter Pin → Tester Ch | `tester_ch = adapter_pin − 1` | adapter — see `ADAPTER_MEZ70.md` |

---

## Identity

| Field | Value |
|-------|-------|
| `id` (ProjectId enum value) | |
| `name` (short human-readable string) | `1x0p5` |

---

## Die Pad to Adapter Pin Cross-Reference

| Die Pad | Signal     | DUT Pin | Adapter Pin | Role |
|---------|------------|---------|---------|------|
| 0  | clk        | 8  | 63 | IO  |
| 1  | rst_n      | 7  | 64 | IO  |
| 2  | bidir_0    | 6  | 65 | IO  |
| 3  | bidir_1    | 5  | 66 | IO  |
| 4  | GND        | 10 | 61 | GND |
| 5  | VDD CORE 1 | 11 | 60 | VCC |
| 6  | bidir_2    | 4  | 67 | IO  |
| 7  | bidir_3    | 3  | 68 | IO  |
| 8  | bidir_4    | 2  | 69 | IO  |
| 9  | bidir_5    | 1  | 70 | IO  |
| 10 | bidir_6    | 70 | 1  | IO  |
| 11 | bidir_7    | 69 | 2  | IO  |
| 12 | bidir_8    | 68 | 3  | IO  |
| 13 | bidir_9    | 67 | 4  | IO  |
| 14 | bidir_10   | 66 | 5  | IO  |
| 15 | bidir_11   | 65 | 6  | IO  |
| 16 | bidir_12   | 64 | 7  | IO  |
| 17 | bidir_13   | 63 | 8  | IO  |
| 18 | GND IO     | 61 | 10 | GND |
| 19 | VDD IO 0   | 62 | 9  | VCC |
| 20 | bidir_14   | 60 | 11 | IO  |
| 21 | bidir_15   | 59 | 12 | IO  |
| 22 | bidir_16   | 58 | 13 | IO  |
| 23 | bidir_17   | 57 | 14 | IO  |
| 24 | GND        | 53 | 18 | GND |
| 25 | PWR Aux 0  | 54 | 17 | VCC |
| 26 | bidir_18   | 56 | 15 | IO  |
| 27 | bidir_19   | 55 | 16 | IO  |
| 28 | bidir_20   | 52 | 19 | IO  |
| 29 | bidir_21   | 51 | 20 | IO  |
| 30 | bidir_22   | 50 | 21 | IO  |
| 31 | bidir_23   | 49 | 22 | IO  |
| 32 | bidir_24   | 48 | 23 | IO  |
| 33 | bidir_25   | 47 | 24 | IO  |
| 34 | GND        | —  | —  | GND (DUT GND plane — no DUT pin) |
| 35 | VDD IO 1   | 44 | 27 | VCC |
| 36 | bidir_26   | 43 | 28 | IO  |
| 37 | bidir_27   | 42 | 29 | IO  |
| 38 | bidir_28   | 41 | 30 | IO  |
| 39 | bidir_29   | 40 | 31 | IO  |
| 40 | VDD CORE   | 46 | 25 | VCC |
| 41 | GND        | 45 | 26 | GND |
| 42 | bidir_30   | 39 | 32 | IO  |
| 43 | bidir_31   | 38 | 33 | IO  |
| 44 | bidir_32   | 37 | 34 | IO  |
| 45 | bidir_33   | 36 | 35 | IO  |
| 46 | bidir_34   | 35 | 36 | IO  |
| 47 | bidir_35   | 34 | 37 | IO  |
| 48 | bidir_36   | 33 | 38 | IO  |
| 49 | bidir_37   | 32 | 39 | IO  |
| 50 | bidir_38   | 31 | 40 | IO  |
| 51 | bidir_39   | 30 | 41 | IO  |
| 52 | bidir_40   | 29 | 42 | IO  |
| 53 | bidir_41   | 28 | 43 | IO  |
| 54 | VDD IO 2   | 24 | 47 | VCC |
| 55 | GND        | 25 | 46 | GND |
| 56 | bidir_42   | 27 | 44 | IO  |
| 57 | bidir_43   | 26 | 45 | IO  |
| 58 | bidir_44   | 23 | 48 | IO  |
| 59 | bidir_45   | 22 | 49 | IO  |
| 60 | PWR Aux 1  | 19 | 52 | VCC |
| 61 | GND        | 18 | 53 | GND |
| 62 | an_0       | 21 | 50 | IO  |
| 63 | an_1       | 20 | 51 | IO  |
| 64 | an_2       | 17 | 54 | IO  |
| 65 | an_3       | 16 | 55 | IO  |
| 66 | in_0       | 12 | 59 | IO  |
| 67 | in_1       | 13 | 58 | IO  |
| 68 | in_2       | 14 | 57 | IO  |
| 69 | in_3       | 15 | 56 | IO  |
| 70 | VDD IO 3   | 9  | 62 | VCC |
| 71 | GND        | —  | —  | GND (DUT GND plane — no DUT pin) |

---

## GND Pads

Six GND pads have individual adapter connector pins; two are bonded to the DUT GND plane only.

| Die Pad | Signal | DUT Pin | Adapter Pin |
|---------|--------|---------|-------------|
| 4  | GND    | 10 | 61 |
| 18 | GND IO | 61 | 10 |
| 24 | GND    | 53 | 18 |
| 41 | GND    | 45 | 26 |
| 55 | GND    | 25 | 46 |
| 61 | GND    | 18 | 53 |

GND plane only (no DUT pin, no adapter pin): die pads 34, 71

```
gndPin = 26  // adapter pin 26 (tester ch 25), die pad 41 — near geometric centre of bond ring
```

---

## IO Adapter Pins in Ring Order

Counter-clockwise from die pad 0 (top-right corner), IO connections only.
Values are **adapter pins** (1-indexed). GND/VCC die pads are skipped.

```
ioAdapterPinsInRingOrder = [
  63, 64, 65, 66,                           // die pads  0– 3: clk, rst_n, bidir_0–1
  67, 68, 69, 70,                           // die pads  6– 9: bidir_2–5
   1,  2,  3,  4,  5,  6,  7,  8,          // die pads 10–17: bidir_6–13
  11, 12, 13, 14,                           // die pads 20–23: bidir_14–17
  15, 16,                                   // die pads 26–27: bidir_18–19
  19, 20, 21, 22, 23, 24,                  // die pads 28–33: bidir_20–25
  28, 29, 30, 31,                           // die pads 36–39: bidir_26–29
  32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, // die pads 42–53: bidir_30–41
  44, 45,                                   // die pads 56–57: bidir_42–43
  48, 49,                                   // die pads 58–59: bidir_44–45
  50, 51,                                   // die pads 62–63: an_0–1
  54, 55,                                   // die pads 64–65: an_2–3
  59, 58, 57, 56                            // die pads 66–69: in_0–3
]
```

---

## Presence Detection

| Die Pad | Signal | DUT Pin | Adapter Pin |
|---------|--------|---------|-------------|
| 18 | GND IO | 61 | 10 |
| 61 | GND    | 18 | 53 |

```
presenceThresholdV = 0.3  // default — calibrate in Phase 3 if needed
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

- 46 bidir pads (bidir_0–45), 4 analog in (an_0–3), 4 digital in (in_0–3), clk, rst_n = 56 IO pads total
- 4 power domains: VDD CORE, VDD CORE 1, VDD IO 0–3, PWR Aux 0–1
- 6 GND pads with individual adapter pins; die pads 34 and 71 are bonded to the DUT GND plane only

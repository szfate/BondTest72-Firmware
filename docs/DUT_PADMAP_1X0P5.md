# DUT Pad Map — 1x0p5

Adapter: 1x0p5 (COB die on 0.5 mm pitch mezzanine adapter)
Source: `2026-05-18 Working out pin mapping.xlsx`, sheet `1x0p5`

Mapping: mez_pad = 71 − die_pad, tester_ch = mez_pad − 1

---

## Identity

| Field | Value |
|-------|-------|
| `id` (ProjectId enum value) | |
| `name` (short human-readable string) | `1x0p5` |

---

## Bond Pad to Tester Channel Cross-Reference

| BP # | Signal | Die Pad | Mez Pad | Tester Ch | Role |
|------|--------|---------|---------|-----------|------|
| 1  | clk        | 8  | 63 | 62 | IO  |
| 2  | rst_n      | 7  | 64 | 63 | IO  |
| 3  | bidir_0    | 6  | 65 | 64 | IO  |
| 4  | bidir_1    | 5  | 66 | 65 | IO  |
| 5  | GND        | 10 | 61 | 60 | GND |
| 6  | VDD CORE 1 | 11 | 60 | 59 | VCC |
| 7  | bidir_2    | 4  | 67 | 66 | IO  |
| 8  | bidir_3    | 3  | 68 | 67 | IO  |
| 9  | bidir_4    | 2  | 69 | 68 | IO  |
| 10 | bidir_5    | 1  | 70 | 69 | IO  |
| 11 | bidir_6    | 70 | 1  | 0  | IO  |
| 12 | bidir_7    | 69 | 2  | 1  | IO  |
| 13 | bidir_8    | 68 | 3  | 2  | IO  |
| 14 | bidir_9    | 67 | 4  | 3  | IO  |
| 15 | bidir_10   | 66 | 5  | 4  | IO  |
| 16 | bidir_11   | 65 | 6  | 5  | IO  |
| 17 | bidir_12   | 64 | 7  | 6  | IO  |
| 18 | bidir_13   | 63 | 8  | 7  | IO  |
| 19 | GND IO     | 61 | 10 | 9  | GND |
| 20 | VDD IO 0   | 62 | 9  | 8  | VCC |
| 21 | bidir_14   | 60 | 11 | 10 | IO  |
| 22 | bidir_15   | 59 | 12 | 11 | IO  |
| 23 | bidir_16   | 58 | 13 | 12 | IO  |
| 24 | bidir_17   | 57 | 14 | 13 | IO  |
| 25 | GND        | 53 | 18 | 17 | GND |
| 26 | PWR Aux 0  | 54 | 17 | 16 | VCC |
| 27 | bidir_18   | 56 | 15 | 14 | IO  |
| 28 | bidir_19   | 55 | 16 | 15 | IO  |
| 29 | bidir_20   | 52 | 19 | 18 | IO  |
| 30 | bidir_21   | 51 | 20 | 19 | IO  |
| 31 | bidir_22   | 50 | 21 | 20 | IO  |
| 32 | bidir_23   | 49 | 22 | 21 | IO  |
| 33 | bidir_24   | 48 | 23 | 22 | IO  |
| 34 | bidir_25   | 47 | 24 | 23 | IO  |
| 35 | GND        | —  | —  | —  | GND (adapter-only, no mez channel) |
| 36 | VDD IO 1   | 44 | 27 | 26 | VCC |
| 37 | bidir_26   | 43 | 28 | 27 | IO  |
| 38 | bidir_27   | 42 | 29 | 28 | IO  |
| 39 | bidir_28   | 41 | 30 | 29 | IO  |
| 40 | bidir_29   | 40 | 31 | 30 | IO  |
| 41 | VDD CORE   | 46 | 25 | 24 | VCC |
| 42 | GND        | 45 | 26 | 25 | GND |
| 43 | bidir_30   | 39 | 32 | 31 | IO  |
| 44 | bidir_31   | 38 | 33 | 32 | IO  |
| 45 | bidir_32   | 37 | 34 | 33 | IO  |
| 46 | bidir_33   | 36 | 35 | 34 | IO  |
| 47 | bidir_34   | 35 | 36 | 35 | IO  |
| 48 | bidir_35   | 34 | 37 | 36 | IO  |
| 49 | bidir_36   | 33 | 38 | 37 | IO  |
| 50 | bidir_37   | 32 | 39 | 38 | IO  |
| 51 | bidir_38   | 31 | 40 | 39 | IO  |
| 52 | bidir_39   | 30 | 41 | 40 | IO  |
| 53 | bidir_40   | 29 | 42 | 41 | IO  |
| 54 | bidir_41   | 28 | 43 | 42 | IO  |
| 55 | VDD IO 2   | 24 | 47 | 46 | VCC |
| 56 | GND        | 25 | 46 | 45 | GND |
| 57 | bidir_42   | 27 | 44 | 43 | IO  |
| 58 | bidir_43   | 26 | 45 | 44 | IO  |
| 59 | bidir_44   | 23 | 48 | 47 | IO  |
| 60 | bidir_45   | 22 | 49 | 48 | IO  |
| 61 | PWR Aux 1  | 19 | 52 | 51 | VCC |
| 62 | GND        | 18 | 53 | 52 | GND |
| 63 | an_0       | 21 | 50 | 49 | IO  |
| 64 | an_1       | 20 | 51 | 50 | IO  |
| 65 | an_2       | 17 | 54 | 53 | IO  |
| 66 | an_3       | 16 | 55 | 54 | IO  |
| 67 | in_0       | 12 | 59 | 58 | IO  |
| 68 | in_1       | 13 | 58 | 57 | IO  |
| 69 | in_2       | 14 | 57 | 56 | IO  |
| 70 | in_3       | 15 | 56 | 55 | IO  |
| 71 | VDD IO 3   | 9  | 62 | 61 | VCC |
| 72 | GND        | —  | —  | —  | GND (adapter-only, no mez channel) |

---

## Pad Roles

Assign one role to every tester channel index 0–71.

| Tester Ch | Role | Signal |
|-----------|------|--------|
| 0  | IO  | bidir_6    |
| 1  | IO  | bidir_7    |
| 2  | IO  | bidir_8    |
| 3  | IO  | bidir_9    |
| 4  | IO  | bidir_10   |
| 5  | IO  | bidir_11   |
| 6  | IO  | bidir_12   |
| 7  | IO  | bidir_13   |
| 8  | VCC | VDD IO 0   |
| 9  | GND | GND IO     |
| 10 | IO  | bidir_14   |
| 11 | IO  | bidir_15   |
| 12 | IO  | bidir_16   |
| 13 | IO  | bidir_17   |
| 14 | IO  | bidir_18   |
| 15 | IO  | bidir_19   |
| 16 | VCC | PWR Aux 0  |
| 17 | GND | GND        |
| 18 | IO  | bidir_20   |
| 19 | IO  | bidir_21   |
| 20 | IO  | bidir_22   |
| 21 | IO  | bidir_23   |
| 22 | IO  | bidir_24   |
| 23 | IO  | bidir_25   |
| 24 | VCC | VDD CORE   |
| 25 | GND | GND        |
| 26 | VCC | VDD IO 1   |
| 27 | IO  | bidir_26   |
| 28 | IO  | bidir_27   |
| 29 | IO  | bidir_28   |
| 30 | IO  | bidir_29   |
| 31 | IO  | bidir_30   |
| 32 | IO  | bidir_31   |
| 33 | IO  | bidir_32   |
| 34 | IO  | bidir_33   |
| 35 | IO  | bidir_34   |
| 36 | IO  | bidir_35   |
| 37 | IO  | bidir_36   |
| 38 | IO  | bidir_37   |
| 39 | IO  | bidir_38   |
| 40 | IO  | bidir_39   |
| 41 | IO  | bidir_40   |
| 42 | IO  | bidir_41   |
| 43 | IO  | bidir_42   |
| 44 | IO  | bidir_43   |
| 45 | GND | GND        |
| 46 | VCC | VDD IO 2   |
| 47 | IO  | bidir_44   |
| 48 | IO  | bidir_45   |
| 49 | IO  | an_0       |
| 50 | IO  | an_1       |
| 51 | VCC | PWR Aux 1  |
| 52 | GND | GND        |
| 53 | IO  | an_2       |
| 54 | IO  | an_3       |
| 55 | IO  | in_3       |
| 56 | IO  | in_2       |
| 57 | IO  | in_1       |
| 58 | IO  | in_0       |
| 59 | VCC | VDD CORE 1 |
| 60 | GND | GND        |
| 61 | VCC | VDD IO 3   |
| 62 | IO  | clk        |
| 63 | IO  | rst_n      |
| 64 | IO  | bidir_0    |
| 65 | IO  | bidir_1    |
| 66 | IO  | bidir_2    |
| 67 | IO  | bidir_3    |
| 68 | IO  | bidir_4    |
| 69 | IO  | bidir_5    |
| 70 | NC  | —          |
| 71 | NC  | —          |

---

## GND Pad

Multiple GND pads are present. Pick one for current injection; the others are noted.

| Tester Ch | Signal | Die Pad |
|-----------|--------|---------|
| 9  | GND IO | 61 |
| 17 | GND    | 53 |
| 25 | GND    | 45 |
| 45 | GND    | 25 |
| 52 | GND    | 18 |
| 60 | GND    | 10 |

```
gndPad = 25   // die pad 45, near geometric centre of bond ring — good current distribution
```

Note: all 6 GND channels are identical to the 1x1 adapter (the mez connector spec fixes their positions). No GND channel is unique to this die.

---

## IO Pads in Physical Ring Order

BP order from the spreadsheet, clockwise starting from BP 1, skipping GND/VCC/NC.

```
ioPadsInRingOrder = [
  62, 63, 64, 65,          // BP 1–4:  clk, rst_n, bidir_0–1
  66, 67, 68, 69,          // BP 7–10: bidir_2–5
  0, 1, 2, 3, 4, 5, 6, 7, // BP 11–18: bidir_6–13
  10, 11, 12, 13,          // BP 21–24: bidir_14–17
  14, 15,                  // BP 27–28: bidir_18–19
  18, 19, 20, 21, 22, 23, // BP 29–34: bidir_20–25
  27, 28, 29, 30,          // BP 37–40: bidir_26–29
  31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, // BP 43–54: bidir_30–41
  43, 44,                  // BP 57–58: bidir_42–43
  47, 48,                  // BP 59–60: bidir_44–45
  49, 50,                  // BP 63–64: an_0–1
  53, 54,                  // BP 65–66: an_2–3
  58, 57, 56, 55           // BP 67–70: in_0–3
]
```

---

## Presence Detection

| Field | Value |
|-------|-------|
| `presencePadA` | 10 (mez pin) |
| `presencePadB` | 53 (mez pin) |
| `presenceThresholdV` | 0.3 (default) |

Same pair used by Mezzanine70 v1/v2. Mez 10 = tester ch 9 (GND, die pad 61); mez 53 = tester ch 52 (GND, die pad 18). Both are GND channels on this adapter.

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
- 6 GND connections on-die; BP 35 and BP 72 are adapter-only GNDs not routed to a mez channel
- Tester channels 70–71 are NC (mez connector capacity exceeds die pad count)

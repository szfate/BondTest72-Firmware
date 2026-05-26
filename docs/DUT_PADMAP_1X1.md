# DUT Pad Map — 1x1

Adapter: 1x1 (COB die on Mezzanine70 connector)
Source: `2026-05-18 Working out pin mapping.xlsx` sheet `1x1`

Mapping: mez_pad = 71 − die_pad, tester_ch = mez_pad − 1

---

## Identity

| Field | Value |
|-------|-------|
| `id` (ProjectId enum value) | |
| `name` (short human-readable string) | `1x1` |

---

## Bond Pad to Tester Channel Cross-Reference

| BP # | Signal | Die Pad | Mez Pad | Tester Ch | Role |
|------|--------|---------|---------|-----------|------|
| 1  | — | 8  | 63 | 62 | IO  |
| 2  | — | 7  | 64 | 63 | IO  |
| 3  | — | 6  | 65 | 64 | IO  |
| 4  | — | 5  | 66 | 65 | IO  |
| 5  | — | 4  | 67 | 66 | IO  |
| 6  | — | 3  | 68 | 67 | IO  |
| 7  | — | 2  | 69 | 68 | IO  |
| 8  | — | 1  | 70 | 69 | IO  |
| 9  | GND IO | —  | —  | —  | GND (bus-only, no mez channel) |
| 10 | — | 70 | 1  | 0  | IO  |
| 11 | — | 69 | 2  | 1  | IO  |
| 12 | — | 68 | 3  | 2  | IO  |
| 13 | — | 67 | 4  | 3  | IO  |
| 14 | — | 66 | 5  | 4  | IO  |
| 15 | — | 65 | 6  | 5  | IO  |
| 16 | — | 64 | 7  | 6  | IO  |
| 17 | — | 63 | 8  | 7  | IO  |
| 18 | VDD IO  | 62 | 9  | 8  | VCC |
| 19 | GND IO  | 61 | 10 | 9  | GND |
| 20 | — | 60 | 11 | 10 | IO  |
| 21 | — | 59 | 12 | 11 | IO  |
| 22 | — | 58 | 13 | 12 | IO  |
| 23 | — | 57 | 14 | 13 | IO  |
| 24 | — | 56 | 15 | 14 | IO  |
| 25 | — | 55 | 16 | 15 | IO  |
| 26 | PWR Aux | 54 | 17 | 16 | VCC |
| 27 | GND IO  | 53 | 18 | 17 | GND |
| 28 | — | 52 | 19 | 18 | IO  |
| 29 | — | 51 | 20 | 19 | IO  |
| 30 | — | 50 | 21 | 20 | IO  |
| 31 | — | 49 | 22 | 21 | IO  |
| 32 | — | 48 | 23 | 22 | IO  |
| 33 | — | 47 | 24 | 23 | IO  |
| 34 | GND IO  | —  | —  | —  | GND (bus-only, no mez channel) |
| 35 | VDD Core | 46 | 25 | 24 | VCC |
| 36 | GND IO  | 45 | 26 | 25 | GND |
| 37 | VDD IO  | 44 | 27 | 26 | VCC |
| 38 | — | 43 | 28 | 27 | IO  |
| 39 | — | 42 | 29 | 28 | IO  |
| 40 | — | 41 | 30 | 29 | IO  |
| 41 | — | 40 | 31 | 30 | IO  |
| 42 | — | 39 | 32 | 31 | IO  |
| 43 | — | 38 | 33 | 32 | IO  |
| 44 | — | 37 | 34 | 33 | IO  |
| 45 | — | 36 | 35 | 34 | IO  |
| 46 | GND IO  | —  | —  | —  | GND (bus-only, no mez channel) |
| 47 | — | 35 | 36 | 35 | IO  |
| 48 | — | 34 | 37 | 36 | IO  |
| 49 | — | 33 | 38 | 37 | IO  |
| 50 | — | 32 | 39 | 38 | IO  |
| 51 | — | 31 | 40 | 39 | IO  |
| 52 | — | 30 | 41 | 40 | IO  |
| 53 | — | 29 | 42 | 41 | IO  |
| 54 | — | 28 | 43 | 42 | IO  |
| 55 | — | 27 | 44 | 43 | IO  |
| 56 | — | 26 | 45 | 44 | IO  |
| 57 | GND IO  | 25 | 46 | 45 | GND |
| 58 | VDD IO  | 24 | 47 | 46 | VCC |
| 59 | — | 23 | 48 | 47 | IO  |
| 60 | — | 22 | 49 | 48 | IO  |
| 61 | — | 21 | 50 | 49 | IO  |
| 62 | — | 20 | 51 | 50 | IO  |
| 63 | GND IO  | 18 | 53 | 52 | GND |
| 64 | PWR Aux | 19 | 52 | 51 | VCC |
| 65 | — | 17 | 54 | 53 | IO  |
| 66 | — | 16 | 55 | 54 | IO  |
| 67 | — | 15 | 56 | 55 | IO  |
| 68 | — | 14 | 57 | 56 | IO  |
| 69 | — | 13 | 58 | 57 | IO  |
| 70 | — | 12 | 59 | 58 | IO  |
| 71 | GND IO  | —  | —  | —  | GND (bus-only, no mez channel) |
| 72 | VDD Core | 11 | 60 | 59 | VCC |
| 73 | GND IO  | 10 | 61 | 60 | GND |
| 74 | VDD IO  | 9  | 62 | 61 | VCC |

---

## Pad Roles

| Tester Ch | Role | Signal |
|-----------|------|--------|
| 0  | IO  | — |
| 1  | IO  | — |
| 2  | IO  | — |
| 3  | IO  | — |
| 4  | IO  | — |
| 5  | IO  | — |
| 6  | IO  | — |
| 7  | IO  | — |
| 8  | VCC | VDD IO   |
| 9  | GND | GND IO   |
| 10 | IO  | — |
| 11 | IO  | — |
| 12 | IO  | — |
| 13 | IO  | — |
| 14 | IO  | — |
| 15 | IO  | — |
| 16 | VCC | PWR Aux  |
| 17 | GND | GND IO   |
| 18 | IO  | — |
| 19 | IO  | — |
| 20 | IO  | — |
| 21 | IO  | — |
| 22 | IO  | — |
| 23 | IO  | — |
| 24 | VCC | VDD Core |
| 25 | GND | GND IO   |
| 26 | VCC | VDD IO   |
| 27 | IO  | — |
| 28 | IO  | — |
| 29 | IO  | — |
| 30 | IO  | — |
| 31 | IO  | — |
| 32 | IO  | — |
| 33 | IO  | — |
| 34 | IO  | — |
| 35 | IO  | — |
| 36 | IO  | — |
| 37 | IO  | — |
| 38 | IO  | — |
| 39 | IO  | — |
| 40 | IO  | — |
| 41 | IO  | — |
| 42 | IO  | — |
| 43 | IO  | — |
| 44 | IO  | — |
| 45 | GND | GND IO   |
| 46 | VCC | VDD IO   |
| 47 | IO  | — |
| 48 | IO  | — |
| 49 | IO  | — |
| 50 | IO  | — |
| 51 | VCC | PWR Aux  |
| 52 | GND | GND IO   |
| 53 | IO  | — |
| 54 | IO  | — |
| 55 | IO  | — |
| 56 | IO  | — |
| 57 | IO  | — |
| 58 | IO  | — |
| 59 | VCC | VDD Core |
| 60 | GND | GND IO   |
| 61 | VCC | VDD IO   |
| 62 | IO  | — |
| 63 | IO  | — |
| 64 | IO  | — |
| 65 | IO  | — |
| 66 | IO  | — |
| 67 | IO  | — |
| 68 | IO  | — |
| 69 | IO  | — |
| 70 | NC  | — |
| 71 | NC  | — |

---

## GND Pad

| Tester Ch | Signal | Die Pad |
|-----------|--------|---------|
| 9  | GND IO | 61 |
| 17 | GND IO | 53 |
| 25 | GND IO | 45 |
| 45 | GND IO | 25 |
| 52 | GND IO | 18 |
| 60 | GND IO | 10 |

```
gndPad = 9   // die pad 61 — used by Mezzanine70 v1/v2 in pad_map_registry.cpp
```

---

## IO Pads in Physical Ring Order

BP order, clockwise, skipping GND/VCC/NC:

```
ioPadsInRingOrder = [
  62, 63, 64, 65, 66, 67, 68, 69,          // BP  1– 8:  die 8–1
   0,  1,  2,  3,  4,  5,  6,  7,          // BP 10–17:  die 70–63
  10, 11, 12, 13, 14, 15,                  // BP 20–25:  die 60–55
  18, 19, 20, 21, 22, 23,                  // BP 28–33:  die 52–47
  27, 28, 29, 30, 31, 32, 33, 34,          // BP 38–45:  die 43–36
  35, 36, 37, 38, 39, 40, 41, 42, 43, 44, // BP 47–56:  die 35–26
  47, 48, 49, 50,                          // BP 59–62:  die 23–20
  53, 54, 55, 56, 57, 58                   // BP 65–70:  die 17–12
]
```

---

## Presence Detection

| Field | Value |
|-------|-------|
| `presencePadA` | 10 (mez pin) |
| `presencePadB` | 53 (mez pin) |
| `presenceThresholdV` | 0.3 (default — calibrate in Phase 3 if needed) |

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

- 56 IO pads (signals undefined — fill in when die datasheet available)
- 6 individually-routed GND pads (ch 9/17/25/45/52/60) + 4 bus-only GND pads (BP 9/34/46/71, no mez channel)
- 4 VDD IO pads, 2 VDD Core pads, 2 PWR Aux pads
- Tester channels 70–71 are the on-adapter self-test diode (NC from die perspective)
- `gndPad = 9` matches existing Mezzanine70 v1/v2 entries in `pad_map_registry.cpp`

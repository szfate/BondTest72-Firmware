# DUT Pad Map — 1x1

Adapter: 1x1 (COB die on Mezzanine70 connector)
Source: `2026-05-18 Working out pin mapping.xlsx` sheet `1x1`
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
| `name` (short human-readable string) | `1x1` |

---

## Die Pad to Adapter Pin Cross-Reference

| Die Pad | Signal   | DUT Pin | Adapter Pin | Role |
|---------|----------|---------|---------|------|
| 0  | IO       | 8  | 63 | IO  |
| 1  | IO       | 7  | 64 | IO  |
| 2  | IO       | 6  | 65 | IO  |
| 3  | IO       | 5  | 66 | IO  |
| 4  | IO       | 4  | 67 | IO  |
| 5  | IO       | 3  | 68 | IO  |
| 6  | IO       | 2  | 69 | IO  |
| 7  | IO       | 1  | 70 | IO  |
| 8  | GND IO   | —  | —  | GND (DUT GND plane — no DUT pin) |
| 9  | IO       | 70 | 1  | IO  |
| 10 | IO       | 69 | 2  | IO  |
| 11 | IO       | 68 | 3  | IO  |
| 12 | IO       | 67 | 4  | IO  |
| 13 | IO       | 66 | 5  | IO  |
| 14 | IO       | 65 | 6  | IO  |
| 15 | IO       | 64 | 7  | IO  |
| 16 | IO       | 63 | 8  | IO  |
| 17 | VDD IO   | 62 | 9  | VCC |
| 18 | GND IO   | 61 | 10 | GND |
| 19 | IO       | 60 | 11 | IO  |
| 20 | IO       | 59 | 12 | IO  |
| 21 | IO       | 58 | 13 | IO  |
| 22 | IO       | 57 | 14 | IO  |
| 23 | IO       | 56 | 15 | IO  |
| 24 | IO       | 55 | 16 | IO  |
| 25 | PWR Aux  | 54 | 17 | VCC |
| 26 | GND IO   | 53 | 18 | GND |
| 27 | IO       | 52 | 19 | IO  |
| 28 | IO       | 51 | 20 | IO  |
| 29 | IO       | 50 | 21 | IO  |
| 30 | IO       | 49 | 22 | IO  |
| 31 | IO       | 48 | 23 | IO  |
| 32 | IO       | 47 | 24 | IO  |
| 33 | GND IO   | —  | —  | GND (DUT GND plane — no DUT pin) |
| 34 | VDD Core | 46 | 25 | VCC |
| 35 | GND IO   | 45 | 26 | GND |
| 36 | VDD IO   | 44 | 27 | VCC |
| 37 | IO       | 43 | 28 | IO  |
| 38 | IO       | 42 | 29 | IO  |
| 39 | IO       | 41 | 30 | IO  |
| 40 | IO       | 40 | 31 | IO  |
| 41 | IO       | 39 | 32 | IO  |
| 42 | IO       | 38 | 33 | IO  |
| 43 | IO       | 37 | 34 | IO  |
| 44 | IO       | 36 | 35 | IO  |
| 45 | GND IO   | —  | —  | GND (DUT GND plane — no DUT pin) |
| 46 | IO       | 35 | 36 | IO  |
| 47 | IO       | 34 | 37 | IO  |
| 48 | IO       | 33 | 38 | IO  |
| 49 | IO       | 32 | 39 | IO  |
| 50 | IO       | 31 | 40 | IO  |
| 51 | IO       | 30 | 41 | IO  |
| 52 | IO       | 29 | 42 | IO  |
| 53 | IO       | 28 | 43 | IO  |
| 54 | IO       | 27 | 44 | IO  |
| 55 | IO       | 26 | 45 | IO  |
| 56 | GND IO   | 25 | 46 | GND |
| 57 | VDD IO   | 24 | 47 | VCC |
| 58 | IO       | 23 | 48 | IO  |
| 59 | IO       | 22 | 49 | IO  |
| 60 | IO       | 21 | 50 | IO  |
| 61 | IO       | 20 | 51 | IO  |
| 62 | GND IO   | 18 | 53 | GND |
| 63 | PWR Aux  | 19 | 52 | VCC |
| 64 | IO       | 17 | 54 | IO  |
| 65 | IO       | 16 | 55 | IO  |
| 66 | IO       | 15 | 56 | IO  |
| 67 | IO       | 14 | 57 | IO  |
| 68 | IO       | 13 | 58 | IO  |
| 69 | IO       | 12 | 59 | IO  |
| 70 | GND IO   | —  | —  | GND (DUT GND plane — no DUT pin) |
| 71 | VDD Core | 11 | 60 | VCC |
| 72 | GND IO   | 10 | 61 | GND |
| 73 | VDD IO   | 9  | 62 | VCC |

---

## GND Pads

Six GND pads have individual adapter connector pins; four are bonded to the DUT GND plane only.

| Die Pad | Signal | DUT Pin | Adapter Pin |
|---------|--------|---------|-------------|
| 18 | GND IO | 61 | 10 |
| 26 | GND IO | 53 | 18 |
| 35 | GND IO | 45 | 26 |
| 56 | GND IO | 25 | 46 |
| 62 | GND IO | 18 | 53 |
| 72 | GND IO | 10 | 61 |

GND plane only (no DUT pin, no adapter pin): die pads 8, 33, 45, 70

```
gndPin = 10  // adapter pin 10 (tester ch 9), die pad 18 — used by Mezzanine70 v1/v2
```

---

## IO Adapter Pins in Ring Order

Counter-clockwise from die pad 0 (top-right corner), IO connections only.
Values are **adapter pins** (1-indexed). GND/VCC die pads are skipped.

```
ioAdapterPinsInRingOrder = [
  63, 64, 65, 66, 67, 68, 69, 70,          // die pads  0– 7
   1,  2,  3,  4,  5,  6,  7,  8,          // die pads  9–16
  11, 12, 13, 14, 15, 16,                  // die pads 19–24
  19, 20, 21, 22, 23, 24,                  // die pads 27–32
  28, 29, 30, 31, 32, 33, 34, 35,          // die pads 37–44
  36, 37, 38, 39, 40, 41, 42, 43, 44, 45, // die pads 46–55
  48, 49, 50, 51,                          // die pads 58–61
  54, 55, 56, 57, 58, 59                   // die pads 64–69
]
```

---

## Presence Detection

| Die Pad | Signal | DUT Pin | Adapter Pin |
|---------|--------|---------|-------------|
| 18 | GND IO | 61 | 10 |
| 62 | GND IO | 18 | 53 |

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

- 56 IO pads (signals undefined — fill in when die datasheet available)
- 6 individually-routed GND pads (adapter pins 10/18/26/46/53/61) + 4 GND-plane-only pads (die pads 8/33/45/70)
- 4 VDD IO pads, 2 VDD Core pads, 2 PWR Aux pads

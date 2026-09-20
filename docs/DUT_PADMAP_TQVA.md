# DUT Pad Map — TQVA

Adapter: Mezzanine70 (same DUT PCB family as 1x1/1x0p5/0p5x1)
Source: user-provided die-pad → DUT pin list, 2026-09-20 (sparse — "..." gaps
between listed pads were filled by linear continuation; verify at bring-up)

Die pad 0 is at the top-right corner; numbers increase counter-clockwise.

**Mapping layers:**

| Layer | Formula | Owner |
|-------|---------|-------|
| Die Pad → DUT Pin | physical bond wires on the DUT PCB | this die |
| DUT Pin → Adapter Pin | `adapter_pin = 71 − dut_pin` | DUT PCB (connector orientation) |
| Adapter Pin → Tester Ch | `tester_ch = adapter_pin − 1` | adapter — see `ADAPTER_MEZ70.md` |

> ⚠️ **WARNING — DUT PIN ≠ ADAPTER PIN.** The source numbers are DUT
> connector pins, NOT adapter pins. Adapter Pin = 71 − dut_pin throughout
> (same formula as 1x1/1x0p5/0p5x1/MOSB).

> **DUT Pin is reference only.** It traces the physical bond wire path and is
> not used in firmware. Only Adapter Pin values appear in `pad_map_registry.cpp`.
> See `GLOSSARY.md` for definitions.

---

## Identity

| Field | Value |
|-------|-------|
| `id` (ProjectId enum value) | 6 |
| `name` (short human-readable string) | `TQVA` |

---

## Die Pad to Adapter Pin Cross-Reference

Fill DUT Pin from the source verbatim; compute Adapter Pin = 71 − dut_pin.
⚠️ See the WARNING above — wrong numbering here silently produces all-garbage
measurement results.

| Die Pad | Signal | DUT Pin | Adapter Pin | Role |
|---------|--------|---------|-------------|------|
| 0  | IO  | 8      | 63      | IO |
| 1  | IO  | 7      | 64      | IO |
| 2  | GND | 10     | 61      | GND |
| 3  | VDDIO | 9 & 11 | 62 & 60 | VCC — bypass cap; bonded to TWO dut pins (shared net — test one, note the other) |
| 4  | IO  | 6      | 65      | IO — continued linearly |
| 5  | IO  | 5      | 66      | IO — continued linearly |
| 6  | IO  | 4      | 67      | IO — continued linearly |
| 7  | IO  | 3      | 68      | IO — continued linearly |
| 8  | IO  | 2      | 69      | IO — continued linearly |
| 9  | IO  | 1      | 70      | IO |
| 10 | IO  | 70     | 1       | IO |
| 11 | IO  | 69     | 2       | IO — continued linearly |
| 12 | IO  | 68     | 3       | IO — continued linearly |
| 13 | IO  | 67     | 4       | IO — continued linearly |
| 14 | IO  | 66     | 5       | IO — continued linearly |
| 15 | IO  | 65     | 6       | IO — continued linearly |
| 16 | IO  | 64     | 7       | IO — continued linearly |
| 17 | IO  | 63     | 8       | IO |
| —  | NC  | 62     | (9)     | NC — between dp17/dp18 in dut numbering |
| —  | GND | 61     | 10      | GND — between dp17/dp18 in dut numbering (die pad not specified in source) |
| 18 | IO  | 60     | 11      | IO |
| 19 | IO  | 59     | 12      | IO — continued linearly |
| 20 | IO  | 58     | 13      | IO — continued linearly |
| 21 | IO  | 57     | 14      | IO — continued linearly |
| 22 | IO  | 56     | 15      | IO — continued linearly |
| 23 | IO  | 55     | 16      | IO — continued linearly |
| 24 | IO  | 52     | 19      | IO |
| 25 | IO  | 51     | 20      | IO |
| —  | GND | 53     | 18      | GND — between dp24/dp25 in dut numbering (die pad not specified in source) |
| 26 | GND | 45     | 26      | GND |
| 27 | VDDIO | 54   | 17      | VCC — bypass cap |
| 28 | IO  | 50     | 21      | IO |
| 29 | IO  | 49     | 22      | IO — continued linearly |
| 30 | IO  | 48     | 23      | IO — continued linearly |
| 31 | IO  | 47     | 24      | IO — continued linearly |
| 32 | IO  | 43     | 28      | IO |
| 33 | —   | —      | —       | NC |
| 34 | —   | —      | —       | NC |
| 35 | VDDIO | 46 & 44 | 25 & 27 | VCC — bypass cap; bonded to TWO dut pins (shared net — test one, note the other) |
| 36 | GND | 25     | 46      | GND |
| 37 | —   | —      | —       | NC |
| 38 | —   | —      | —       | NC |
| 39 | IO  | 12     | 59      | IO |
| 40 | IO  | 13     | 58      | IO |
| 41 | VDD_CORE | 24 | 47     | VCC — bypass cap |
| 42 | GND | 18     | 53      | GND |
| 43 | —   | —      | —       | NC |
| 44 | —   | —      | —       | NC |
| 45 | IO  | 42     | 29      | IO |
| 46 | IO  | 41     | 30      | IO — continued linearly |
| 47 | IO  | 40     | 31      | IO — continued linearly |
| 48 | IO  | 39     | 32      | IO — continued linearly |
| 49 | IO  | 38     | 33      | IO — continued linearly |
| 50 | IO  | 37     | 34      | IO — continued linearly |
| 51 | —   | —      | —       | NC |
| 52 | —   | —      | —       | NC |
| 53 | —   | —      | —       | NC |
| 54 | —   | —      | —       | NC |
| 55 | —   | —      | —       | NC |

---

## GND Pads

Six GND pads, each with a routed adapter pin — the classic GND↔GND presence
short is possible. Pick one for current injection (gndPin); note the others.

| Die Pad | Signal | DUT Pin | Adapter Pin |
|---------|--------|---------|-------------|
| —  | GND | 61 | 10 |
| —  | GND | 53 | 18 |
| 26 | GND | 45 | 26 |
| 36 | GND | 25 | 46 |
| 42 | GND | 18 | 53 |
| 2  | GND | 10 | 61 |

Note: dut pins 61 and 53 carry GND but their die pads were not specified in
the source list — they are recorded here by dut pin only. The GND adapter-pin
set (10, 18, 26, 46, 53, 61) is identical to the 1x1/1x0p5 mezzanine set,
which corroborates the numbering.

GND plane only (no DUT pin, no adapter pin): none noted.

```
gndPin = 53  // adapter pin 53 (tester ch 52), die pad 42 — TBD after hardware bring-up
```

---

## IO Adapter Pins in Ring Order

Counter-clockwise from die pad 0 (top-right corner), IO connections only.
Values are **adapter pins** (1-indexed). GND/VCC/NC die pads are skipped.

```
ioAdapterPinsInRingOrder = [
   63, 64, 65, 66, 67, 68, 69, 70,  // die pads  0, 1, 4– 9
    1,  2,  3,  4,  5,  6,  7,  8,  // die pads 10–17
   11, 12, 13, 14, 15, 16,          // die pads 18–23
   19, 20,                          // die pads 24–25
   21, 22, 23, 24,                  // die pads 28–31
   28,                              // die pad  32
   59, 58,                          // die pads 39–40
   29, 30, 31, 32, 33, 34           // die pads 45–50
]
```

---

## Presence Detection

Six GND adapter pins are available (10, 18, 26, 46, 53, 61). Proposed pair —
same as the 1x1/1x0p5 maps; verify at bring-up that both are always connected
and that the flipped mirror pair reads open (for the orientation check):

| Die Pad | Signal | DUT Pin | Adapter Pin |
|---------|--------|---------|-------------|
| —  | GND | 10 | 61 |
| —  | GND | 18 | 53 |

```
presenceThresholdV = 0.3  // default — calibrate in Phase 3 if needed
```

---

## Test Thresholds

Leave blank until Phase 3 hardware calibration. Defaults from ARCHITECTURE.md
are shown; override here if this die's process differs.

| Signal | Threshold | Default | This die |
|--------|-----------|---------|----------|
| COM_D good bond min | V | 0.50 V | |
| COM_D good bond max | V | 0.70 V | |
| COM_D open bond min | V | 2.50 V | |
| COM_D short max | V | 0.10 V | |
| COM_A / COM_C short max | V | 0.30 V | |

---

## Notes

- 37 IO pads + 3 VDDIO + 1 VDD_CORE = **41 test cases** (VDD pads all carry
  bypass caps → CAP_SENSE); 6 GND dut pins (61/53/45/25/18/10 → apins
  10/18/26/46/53/61); NC dut pins: 62 (apin 9) and the 9 NC die pads
  (33/34/37/38/43/44/51–55).
- **Shared-net VDD pads**: die pad 3 is bonded to two dut pins (9 & 11 →
  apins 62 & 60) and die pad 35 to two (46 & 44 → apins 25 & 27). One
  measurement covers both bonds per pad, but the untested sibling pin is on
  the same PCB net — a broken bond on it cannot be distinguished through the
  shared bypass cap (see kelvin.h note on shared-PCB-net power pins). The
  cross-reference lists one adapter pin per VDD case; the sibling is noted.
- **Die pads for GND dut pins 61 and 53 were not specified in the source**
  (they sit between dp17/dp18 and dp24/dp25 in dut numbering). Recorded by
  dut pin only — this does not block the registry translation, since GND
  connections are used only via their adapter pins in firmware.
- "..." gaps were filled by linear continuation (same convention as the
  0p5x1 map); every such row is marked "continued linearly" above.

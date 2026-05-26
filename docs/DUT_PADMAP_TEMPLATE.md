# DUT Pad Map — [Project Name]

Copy this file to `DUT_PADMAP_<PROJECT>.md` and fill in every blank field.
When complete, translate into a `PadMap` entry in `src/test/pad_map_registry.cpp`.

Adapter: [adapter name]
Source: [spreadsheet / schematic reference]

Mapping: mez_pad = 71 − die_pad, tester_ch = mez_pad − 1

---

## Identity

| Field | Value |
|-------|-------|
| `id` (ProjectId enum value) | |
| `name` (short human-readable string) | |

---

## Bond Pad to Tester Channel Cross-Reference

| BP # | Signal | Die Pad | Mez Pad | Tester Ch | Role |
|------|--------|---------|---------|-----------|------|
| 1  | | | | | |
| 2  | | | | | |
| …  | | | | | |

---

## Pad Roles

| Tester Ch | Role | Signal |
|-----------|------|--------|
| 0  | | |
| 1  | | |
| 2  | | |
| 3  | | |
| 4  | | |
| 5  | | |
| 6  | | |
| 7  | | |
| 8  | | |
| 9  | | |
| 10 | | |
| 11 | | |
| 12 | | |
| 13 | | |
| 14 | | |
| 15 | | |
| 16 | | |
| 17 | | |
| 18 | | |
| 19 | | |
| 20 | | |
| 21 | | |
| 22 | | |
| 23 | | |
| 24 | | |
| 25 | | |
| 26 | | |
| 27 | | |
| 28 | | |
| 29 | | |
| 30 | | |
| 31 | | |
| 32 | | |
| 33 | | |
| 34 | | |
| 35 | | |
| 36 | | |
| 37 | | |
| 38 | | |
| 39 | | |
| 40 | | |
| 41 | | |
| 42 | | |
| 43 | | |
| 44 | | |
| 45 | | |
| 46 | | |
| 47 | | |
| 48 | | |
| 49 | | |
| 50 | | |
| 51 | | |
| 52 | | |
| 53 | | |
| 54 | | |
| 55 | | |
| 56 | | |
| 57 | | |
| 58 | | |
| 59 | | |
| 60 | | |
| 61 | | |
| 62 | | |
| 63 | | |
| 64 | | |
| 65 | | |
| 66 | | |
| 67 | | |
| 68 | | |
| 69 | | |
| 70 | NC | — |
| 71 | NC | — |

---

## GND Pad

Multiple GND pads are typically present. Pick one for current injection; note the others.

| Tester Ch | Signal | Die Pad |
|-----------|--------|---------|
| | | |

```
gndPad =   (pick one — TBD after hardware bring-up)
```

---

## IO Pads in Physical Ring Order

BP order from the spreadsheet, clockwise, skipping GND/VCC/NC.

```
ioPadsInRingOrder = [

]
```

---

## Presence Detection

The adapter PCB shorts two GND mez channels together. When seated, continuity
between them is detectable.

| Field | Value |
|-------|-------|
| `presencePadA` | 10 (mez pin) |
| `presencePadB` | 53 (mez pin) |
| `presenceThresholdV` | 0.3 (default — calibrate in Phase 3 if needed) |

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

<!-- IO count, power domains, any die-specific quirks or bonding exceptions -->

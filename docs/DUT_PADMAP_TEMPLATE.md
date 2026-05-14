# DUT Pad Map — [Project Name]

Copy this file to `DUT_PADMAP_<PROJECT>.md` and fill in every field.
When complete, translate into a `PadMap` entry in `src/test/pad_map_registry.cpp`.

---

## Identity

| Field | Value |
|-------|-------|
| `id` (ProjectId enum value) | |
| `name` (short human-readable string) | |

---

## Pad Roles

Assign one role to every pad index 0–71.
Unused pads on this die (pad indices not bonded out) → **NC**.

Roles:
- **IO** — bonded signal pad (will be tested)
- **GND** — die ground pad (current injection point — must match `gndPad` below)
- **VCC** — die power pad (excluded from bond test)
- **NC** — not connected / no bond on this adapter

| Pad Index | Role (IO / GND / VCC / NC) | Notes |
|-----------|---------------------------|-------|
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
| 70 | | |
| 71 | | |

---

## GND Pad

The pad index used for current injection (must have role GND above).

```
gndPad = 
```

---

## IO Pads in Physical Ring Order

List **only IO-role pad indices**, in the order they appear around the die pad ring
(e.g. clockwise starting from pad 1). This defines the neighbour pairs used for
adjacent-bond short detection.

Do **not** include GND, VCC, or NC pads here.

```
ioPadsInRingOrder = [ ]
```

Example: `[ 0, 1, 2, 5, 6, 7, 8, 10, 11 ]`

---

## Presence Detection

Two pad indices that are shorted together through the DUT PCB (adapter routing).
When a DUT is seated, continuity between these pads is detectable — voltage on
COM_A drops below `presenceThresholdV`.

Choose a pair of NC or GND pads that the adapter PCB connects together so the
tester can confirm a die is physically present without consuming IO pads.

| Field | Value |
|-------|-------|
| `presencePadA` | |
| `presencePadB` | |
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

<!-- Any die-specific quirks, special pads, bonding exceptions, etc. -->

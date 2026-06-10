# Adapter — Mezzanine70

**Hardware:** Mezzanine70 (hw = 0x01)

The Mezzanine70 adapter connects a 70-pin DUT PCB connector to the BondTest72
tester. It does not contain active components or isolation switches.

---

## Adapter Pin → Tester Channel

Adapter pins are 1-indexed (physical connector labels). Tester channels are
0-indexed firmware indices used in `TestCase.mezPin` calls via `channelForPin()`.

```
tester_ch = adapter_pin − 1
```

Full reference:

| Adapter Pin | Tester Ch | | Adapter Pin | Tester Ch | | Adapter Pin | Tester Ch |
|-------------|-----------|---|-------------|-----------|---|-------------|-----------|
| 1  | 0  | | 25 | 24 | | 49 | 48 |
| 2  | 1  | | 26 | 25 | | 50 | 49 |
| 3  | 2  | | 27 | 26 | | 51 | 50 |
| 4  | 3  | | 28 | 27 | | 52 | 51 |
| 5  | 4  | | 29 | 28 | | 53 | 52 |
| 6  | 5  | | 30 | 29 | | 54 | 53 |
| 7  | 6  | | 31 | 30 | | 55 | 54 |
| 8  | 7  | | 32 | 31 | | 56 | 55 |
| 9  | 8  | | 33 | 32 | | 57 | 56 |
| 10 | 9  | | 34 | 33 | | 58 | 57 |
| 11 | 10 | | 35 | 34 | | 59 | 58 |
| 12 | 11 | | 36 | 35 | | 60 | 59 |
| 13 | 12 | | 37 | 36 | | 61 | 60 |
| 14 | 13 | | 38 | 37 | | 62 | 61 |
| 15 | 14 | | 39 | 38 | | 63 | 62 |
| 16 | 15 | | 40 | 39 | | 64 | 63 |
| 17 | 16 | | 41 | 40 | | 65 | 64 |
| 18 | 17 | | 42 | 41 | | 66 | 65 |
| 19 | 18 | | 43 | 42 | | 67 | 66 |
| 20 | 19 | | 44 | 43 | | 68 | 67 |
| 21 | 20 | | 45 | 44 | | 69 | 68 |
| 22 | 21 | | 46 | 45 | | 70 | 69 |
| 23 | 22 | | 47 | 46 | | —  | 70 (NC) |
| 24 | 23 | | 48 | 47 | | —  | 71 (NC) |

Tester channels 70–71 are the on-adapter self-test diode; no DUT adapter pin
maps to them.

---

## Presence Detection Mechanism

The Mezzanine70 PCB shorts two adapter pins together via a trace. When a DUT is
seated, its GND network closes the circuit — firmware detects continuity.

Which two adapter pins to monitor is **DUT-specific** (they must both be GND pins
on the DUT). See the `presencePadA` / `presencePadB` fields in each
`DUT_PADMAP_<project>.md`.

---

## Flipped-DUT Detection

A DUT inserted 180° rotated maps each adapter pin to `71 − adapter_pin`. Firmware
checks the flipped presence pair automatically (see `mezzanine70.cpp`).

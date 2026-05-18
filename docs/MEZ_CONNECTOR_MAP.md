# Mezzanine70 Connector Map

70-pin 0.4 mm mezzanine connector. Left column top-to-bottom (pins 1–35),
right column bottom-to-top (pins 36–70) — snake routing.

**All pin numbers here are adapter-side mezzanine pin numbers** (what the tester sees).
The COB-side numbering is mirrored — do not use COB-side pin numbers in firmware.

**Tester channel = mezzanine pin − 1**  (0-indexed logical pad)

Tester channels 70–71: on-adapter diode (adapter self-test, not a die pad).

---

## Pin Table

Type key: **IO** = testable bond pad · **GND** = die GND pad (all shorted to GND bus on adapter) · **PWR** = power supply rail

| Mez | Ch | Type     | Die pad | Signal |
|----:|---:|----------|--------:|--------|
|   1 |  0 | IO       |      10 |        |
|   2 |  1 | IO       |      11 |        |
|   3 |  2 | IO       |      12 |        |
|   4 |  3 | IO       |      13 |        |
|   5 |  4 | IO       |      14 |        |
|   6 |  5 | IO       |      15 |        |
|   7 |  6 | IO       |      16 |        |
|   8 |  7 | IO       |      17 |        |
|   9 |  8 | VDDIO    |      18 |        |
|  10 |  9 | GND      |      19 |        |
|  11 | 10 | IO       |      20 |        |
|  12 | 11 | IO       |      21 |        |
|  13 | 12 | IO       |      22 |        |
|  14 | 13 | IO       |      23 |        |
|  15 | 14 | IO       |      24 |        |
|  16 | 15 | IO       |      25 |        |
|  17 | 16 | PWR_AUX  |      26 |        |
|  18 | 17 | GND      |      27 |        |
|  19 | 18 | IO       |      28 |        |
|  20 | 19 | IO       |      29 |        |
|  21 | 20 | IO       |      30 |        |
|  22 | 21 | IO       |      31 |        |
|  23 | 22 | IO       |      32 |        |
|  24 | 23 | IO       |      33 |        |
|   — |    | GND      |      34 |        |
|  25 | 24 | VDD_CORE |      35 |        |
|  26 | 25 | GND      |      36 |        |
|  27 | 26 | VDDIO    |      37 |        |
|  28 | 27 | IO       |      38 |        |
|  29 | 28 | IO       |      39 |        |
|  30 | 29 | IO       |      40 |        |
|  31 | 30 | IO       |      41 |        |
|  32 | 31 | IO       |      42 |        |
|  33 | 32 | IO       |      43 |        |
|  34 | 33 | IO       |      44 |        |
|  35 | 34 | IO       |      45 |        |
|   — |    | GND      |      46 |        |
|  36 | 35 | IO       |      47 |        |
|  37 | 36 | IO       |      48 |        |
|  38 | 37 | IO       |      49 |        |
|  39 | 38 | IO       |      50 |        |
|  40 | 39 | IO       |      51 |        |
|  41 | 40 | IO       |      52 |        |
|  42 | 41 | IO       |      53 |        |
|  43 | 42 | IO       |      54 |        |
|  44 | 43 | IO       |      55 |        |
|  45 | 44 | IO       |      56 |        |
|  46 | 45 | GND      |      57 |        |
|  47 | 46 | VDDIO    |      58 |        |
|  48 | 47 | IO       |      59 |        |
|  49 | 48 | IO       |      60 |        |
|  50 | 49 | IO       |      61 |        |
|  51 | 50 | IO       |      62 |        |
|  52 | 51 | PWR_AUX  |      64 |        |
|  53 | 52 | GND      |      63 |        |
|  54 | 53 | IO       |      65 |        |
|  55 | 54 | IO       |      66 |        |
|  56 | 55 | IO       |      67 |        |
|  57 | 56 | IO       |      68 |        |
|  58 | 57 | IO       |      69 |        |
|  59 | 58 | IO       |      70 |        |
|   — |    | GND      |      71 |        |
|  60 | 59 | VDD_CORE |      72 |        |
|  61 | 60 | GND      |      73 |        |
|  62 | 61 | VDDIO    |      74 |        |
|  63 | 62 | IO       |       1 |        |
|  64 | 63 | IO       |       2 |        |
|  65 | 64 | IO       |       3 |        |
|  66 | 65 | IO       |       4 |        |
|  67 | 66 | IO       |       5 |        |
|  68 | 67 | IO       |       6 |        |
|  69 | 68 | IO       |       7 |        |
|  70 | 69 | IO       |       8 |        |
|   — |    | GND      |       9 |        |

---

## Die pad summary

**IO pads (56):** 1–8, 10–17, 20–25, 28–33, 38–45, 47–56, 59–62, 65–70

**GND pads (10):** 9, 19, 27, 34, 36, 46, 57, 63, 71, 73
- Pads 19, 27, 36, 57, 63, 73: each has an individual mez signal pin (all tied to GND bus on adapter)
- Pads 9, 34, 46, 71: connect only through the 6 dedicated GND bus pins (no individual signal line)

**VDDIO pads (4):** 18, 37, 58, 74

**VDD_CORE pads (2):** 35, 72

**PWR_AUX pads (2):** 26, 64

---

## Notes for TestCase definition

- `gnd` field: use the mez pin of the nearest die GND pad
  (mez 10/18/26/46/53/61 — all electrically equivalent on the adapter)
- `pad` field: use the mez pin of the IO pad under test

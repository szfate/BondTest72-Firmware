# Mezzanine70 Connector Map

70-pin 0.4 mm mezzanine connector. Left column top-to-bottom (pins 1–35),
right column bottom-to-top (pins 36–70) — snake routing.

**Tester channel = mezzanine pin − 1**  (0-indexed logical pad)

Tester channels 70–71: on-adapter diode (adapter self-test, not a die pad).

---

## Pin Table

Type key: **IO** = testable bond pad · **GND** = die GND pad (all shorted to GND bus on adapter) · **PWR** = power supply rail

| Mez | Ch | Type     | Die pad | Signal |
|----:|---:|----------|--------:|--------|
|   1 |  0 | IO       |       8 |        |
|   2 |  1 | IO       |       7 |        |
|   3 |  2 | IO       |       6 |        |
|   4 |  3 | IO       |       5 |        |
|   5 |  4 | IO       |       4 |        |
|   6 |  5 | IO       |       3 |        |
|   7 |  6 | IO       |       2 |        |
|   8 |  7 | IO       |       1 |        |
|   9 |  8 | VDDIO    |      74 |        |
|  10 |  9 | GND      |      73 |        |
|  11 | 10 | VDD_CORE |      72 |        |
|   — |    | GND      |      71 |        |
|  12 | 11 | IO       |      70 |        |
|  13 | 12 | IO       |      69 |        |
|  14 | 13 | IO       |      68 |        |
|  15 | 14 | IO       |      67 |        |
|  16 | 15 | IO       |      66 |        |
|  17 | 16 | IO       |      65 |        |
|  18 | 17 | GND      |      63 |        |
|  19 | 18 | PWR_AUX  |      64 |        |
|  20 | 19 | IO       |      62 |        |
|  21 | 20 | IO       |      61 |        |
|  22 | 21 | IO       |      60 |        |
|  23 | 22 | IO       |      59 |        |
|  24 | 23 | VDDIO    |      58 |        |
|  25 | 24 | GND      |      57 |        |
|  26 | 25 | IO       |      56 |        |
|  27 | 26 | IO       |      55 |        |
|  28 | 27 | IO       |      54 |        |
|  29 | 28 | IO       |      53 |        |
|  30 | 29 | IO       |      52 |        |
|  31 | 30 | IO       |      51 |        |
|  32 | 31 | IO       |      50 |        |
|  33 | 32 | IO       |      49 |        |
|  34 | 33 | IO       |      48 |        |
|  35 | 34 | IO       |      47 |        |
|   — |    | GND      |      46 |        |
|  36 | 35 | IO       |      45 |        |
|  37 | 36 | IO       |      44 |        |
|  38 | 37 | IO       |      43 |        |
|  39 | 38 | IO       |      42 |        |
|  40 | 39 | IO       |      41 |        |
|  41 | 40 | IO       |      40 |        |
|  42 | 41 | IO       |      39 |        |
|  43 | 42 | IO       |      38 |        |
|  44 | 43 | VDDIO    |      37 |        |
|  45 | 44 | GND      |      36 |        |
|  46 | 45 | VDD_CORE |      35 |        |
|   — |    | GND      |      34 |        |
|  47 | 46 | IO       |      33 |        |
|  48 | 47 | IO       |      32 |        |
|  49 | 48 | IO       |      31 |        |
|  50 | 49 | IO       |      30 |        |
|  51 | 50 | IO       |      29 |        |
|  52 | 51 | IO       |      28 |        |
|  53 | 52 | GND      |      27 |        |
|  54 | 53 | PWR_AUX  |      26 |        |
|  55 | 54 | IO       |      25 |        |
|  56 | 55 | IO       |      24 |        |
|  57 | 56 | IO       |      23 |        |
|  58 | 57 | IO       |      22 |        |
|  59 | 58 | IO       |      21 |        |
|  60 | 59 | IO       |      20 |        |
|  61 | 60 | GND      |      19 |        |
|  62 | 61 | VDDIO    |      18 |        |
|  63 | 62 | IO       |      17 |        |
|  64 | 63 | IO       |      16 |        |
|  65 | 64 | IO       |      15 |        |
|  66 | 65 | IO       |      14 |        |
|  67 | 66 | IO       |      13 |        |
|  68 | 67 | IO       |      12 |        |
|  69 | 68 | IO       |      11 |        |
|  70 | 69 | IO       |      10 |        |
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
  (mez 10/18/25/45/53/61 — all electrically equivalent on the adapter)
- `pad` field: use the mez pin of the IO pad under test

# BondTest72 — What to Provide Before Coding Can Proceed

Items are ordered by dependency. Each phase unblocks the next.

---

## Phase 1 — Unblocks HAL drivers (hard blockers)

- [x] **Finalized schematic** — pin assignments captured in `docs/RP2350 PINMAP.md`
  - GP1  → SK6812 DIN
  - CH446X bit-bang serial: DAT1/CLK1/STB1 (GP13/14/12), DAT2/CLK2/STB2 (GP10/11/5), DAT3/CLK3/STB3 (GP8/9/4), RST shared (GP7)
  - GP26/ADC0 → COM_D (injection+sense, 27 kΩ pullup)
  - GP27/ADC1 → COM_A (neighbour sense, 1 MΩ divider)
  - GP28/ADC2 → COM_C (neighbour sense, 1 MΩ divider)
  - GP17 → AT21CS01 SWI (CON6)
  - GP0  → START button (active low, internal pullup)
  - GP6  → NC (spare)
  - GP15/22/18/19/20/21 → CON0–CON5 (adapter control); CON6 = EEPROM SWI
  - Still needed: which adapter-control line drives the adapter ID LED

- [x] **CH446X datasheet** — provided in English and Mandarin. Note: datasheet covers
  both CH446Q and CH446X variants; schematic uses CH446X — confirm any behavioural
  differences between variants before writing the abit-bang serial driver.

- [x] **AT21CS01 datasheet** — provided.

- [x] **MUX channel mapping table** — completed in `docs/MUX_MAP.md`.
  Chips are U2/U3/U4 (no U1 — likely the Pico module). Logical pad = CH net number.

---

## Phase 2 — Unblocks test engine

- [ ] **Pad map definitions** (×4) — for each known die project, provide:
  - Pad roles: IO / GND / VCC / NC for each of the 72 pad indices
  - Physical pad ring order (determines neighbour pairs for adjacent-bond testing)
  - GND pad index (current injection point)
  - Two pads that are shorted through the DUT PCB (for presence detection)

- [ ] **Adapter EEPROM initial values** for Mezzanine70 — needed to define the
  programming fixture / factory script:
  - `adapter_model` byte value for Mezzanine70 (e.g. `0x01`)
  - `adapter_version` for first production run
  - `supported_padmap_id` — which of the 4 pad maps this adapter carries
  - `designed_lifespan` — rated insertion count for the 70-pin 0.4 mm mezzanine connector

---

## Phase 3 — Calibration (requires assembled hardware)

- [ ] **COM_D voltage thresholds** — measure on real silicon to confirm:
  - Good bond range (expected ~0.5–0.7 V, process-dependent)
  - Open bond threshold (expected >~2.5 V)
  - Short threshold (expected <~0.1 V)

- [ ] **COM_A / COM_C neighbour sense thresholds** — measure with and without a
  deliberate neighbour short to confirm the 1 MΩ/1 MΩ divider midpoint and short
  detection threshold (expected ~1.65 V no-short, <~0.3 V short)

- [ ] **Presence detection threshold** — measure COM_D with and without DUT seated
  to confirm the continuity voltage level for the chosen presence pad pair

---

## Phase 4 — Test framework refactor (in progress)

Motivation: global per-PadMap thresholds are too coarse; capacitive power pads need
a different measurement strategy; a pre-measurement discharge step requires a no-result
"nop" case type. The goal is per-TestCase flexibility without future rewrites.

### Data model changes

**New `TestThresholds` struct** (shareable, define once, reference from many cases):
```cpp
struct TestThresholds {
    float senseGoodMin;
    float senseGoodMax;
    float neighbourGoodMin;
    float neighbourGoodMax;
};
```

**New `TestStrategy` enum**:
```cpp
enum class TestStrategy : uint8_t {
    STANDARD,    // normal: sense + neighbours
    SKIP_SENSE,  // capacitive pad: neighbours only, bond sense not checked
    DISCHARGE,   // nop: connect buses for settleMs, no result recorded
};
```

**`TestCase` additions** — `strategy`, `settleMs`, `thresholds` pointer:
```cpp
struct TestCase {
    uint8_t              mezPin;
    uint8_t              gndPin;
    uint8_t              prevPin;
    uint8_t              nextPin;
    uint8_t              diePad;
    TestStrategy         strategy;   // default STANDARD
    uint16_t             settleMs;   // per-case settle, replaces hardcoded delay(2)
    const TestThresholds* thresholds; // nullptr only valid for DISCHARGE
};
```

**`PadMap`** — remove `senseGoodMin/Max` and `neighbourGoodMin/Max`; presence
detection fields stay (they are pad-map-level, not per-bond).

### Runner changes (`test_runner.cpp`)

- `classify()` takes `const TestThresholds&` instead of `const PadMap&`
- Loop per case:
  - `DISCHARGE` → connect buses, `delay(settleMs)`, skip classify, skip result counters
  - `SKIP_SENSE` → connect, wait, classify neighbours only (bond = GOOD unconditionally)
  - `STANDARD`  → existing behaviour, using `tc.thresholds`

### Registry changes (`pad_map_registry.cpp`)

- Define a small set of shared `TestThresholds` instances:
  - `kThreshIO`  — standard IO bond range
  - `kThreshPwr` — power pad range (wider or SKIP_SENSE)
- Update all `TestCase` initialisers to carry `strategy`, `settleMs`, `thresholds`
- Capacitive power pads (die64 PWR_AUX, die26 PWR_AUX, etc.) get `SKIP_SENSE` + appropriate `settleMs`
- Pre-discharge steps inserted before capacitive cases as `DISCHARGE` entries (both prevPin/nextPin set to gndPin, no diePad logged)
- `buildRingCases()` gains `strategy`, `settleMs`, `thresholds` params (or sensible defaults)

### Files to change

- [x] `src/test/pad_map.h` — add `TestThresholds`, `TestStrategy`, extend `TestCase`, remove global thresholds from `PadMap`
- [x] `src/test/pad_map.cpp` — update `buildRingCases` signature
- [x] `src/test/test_runner.cpp` — strategy dispatch in loop, `classify()` signature
- [x] `src/test/pad_map_registry.cpp` — define threshold instances, update all cases
- [x] `src/app/host_protocol.cpp` / result reporting — no struct change needed

---

## Phase 5 — Future adapter (not needed for MVP)

- [ ] **Mezzanine70x5 GPIO mapping** — which of the 7 adapter-control GPIOs drives
  which CBT16211 isolation switch for each of the 5 DUT slots

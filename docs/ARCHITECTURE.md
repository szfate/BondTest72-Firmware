# BondTest72 Firmware Architecture

> Full hardware context: `BondTest72_Overview.md` in the project OneDrive folder.

---

## Hardware Summary (firmware-relevant)

| Item | Detail |
|------|--------|
| MCU | RP2350 (Raspberry Pi Pico 2) |
| Muxes | 3× CH446X (24:5 matrix), covering 72 channels |
| LEDs | 3× SK6812 addressable RGB, single GPIO, series chain |
| Buttons | 2× start, 1× reset |
| EEPROM | AT21CS01 (Microchip Single-Wire Interface — **not** Dallas/Maxim 1-Wire) |
| ADC | Internal RP2350 ADC; reads COM_D (ADC0/GP26), COM_A (ADC1/GP27), COM_C (ADC2/GP28) — only COM_A (Kelvin sense) is used by the production bond-test path; COM_D/COM_C are debug-only |
| USB | CDC serial to host PC |
| Adapter connector | 2×41 pin (82 pins): 72 measurement channels + 7 adapter-control GPIOs + power/GND |

The last pin of CON6 (adapter header) is dedicated to the AT21CS01 SWI data line.

---

## Layer Diagram

```
┌──────────────────────────────────────────────────────────┐
│                    Application Layer                     │
│     StateMachine  ·  HostProtocol  ·  LedManager         │
├──────────────────────────┬───────────────────────────────┤
│       Test Engine        │       Adapter Layer           │
│  TestRunner  PadMap      │  AdapterBase (interface)      │
│  PadMapRegistry          │  Mezzanine70 / Mezzanine70r2  │
│  DutDetector             │  AdapterRegistry               │
│  DiscoveryScanner        │  EepromManager                 │
├──────────────────────────┴───────────────────────────────┤
│               Hardware Abstraction Layer (HAL)           │
│   MuxController · AdcDriver · Kelvin (measurement)        │
│   SK6812Controller · Buttons · AT21CS01Driver             │
├──────────────────────────────────────────────────────────┤
│                    RP2350 / Arduino                     │
└──────────────────────────────────────────────────────────┘
```

---

## Directory Structure

```
src/
├── main.cpp                          # entry point — constructs and wires all layers
│
├── hal/
│   ├── mux.h / mux.cpp              # 3× CH446X bit-bang serial driver: setChannel(logicalPad, bus), clearAll()
│   ├── mux_map.h / mux_map.cpp      # MUX_MAP[72]: logical pad → (chip, channel); filled from schematic
│   ├── adc.h / adc.cpp              # ADC wrapper: readVoltage()
│   ├── kelvin.h / kelvin.cpp        # Kelvin resistance-sweep primitives (measureKelvin, measureKelvinCurve)
│   ├── sk6812.h / sk6812.cpp        # addressable LED driver, 3 pixels, single GPIO
│   ├── buttons.h / buttons.cpp      # debounced start button input
│   └── at21cs01.h / at21cs01.cpp    # Microchip SWI EEPROM driver (NOT Dallas 1-Wire)
│
├── adapter/
│   ├── adapter_base.h               # pure abstract interface
│   ├── eeprom_layout.h / .cpp       # EepromData struct + serialize/deserialize
│   ├── eeprom_manager.h / .cpp      # presence check, CRC-checked read/write, serial UID read
│   ├── mezzanine70.h / .cpp         # 1-DUT adapter (Mezzanine70 r1/r2 — r2 overrides only selfTest())
│   └── adapter_registry.h / .cpp   # (hwId) → AdapterBase* factory
│
├── test/
│   ├── pad_map.h                            # TestCase/PadMap structs, TestStrategy/PadType enums
│   ├── pad_map_registry.h / .cpp            # hardcoded PadMap instances (see PadMapRegistry below)
│   ├── dut_detector.h / dut_detector.cpp   # debounced presence/orientation polling via PadMap recipe
│   ├── test_runner.h / test_runner.cpp      # test orchestration, adapter-agnostic
│   ├── discovery_scanner.h / .cpp           # 70×70 pad-to-pad voltage sweep (unknown pad map)
│   └── result.h                             # result structs (per-pad, per-slot, per-run)
│
└── app/
    ├── state.h                      # State enum
    ├── state_machine.h / .cpp       # top-level states, drives LEDs/buttons; also owns adapter-liveness
    │                                # and DUT polling (no separate AdapterMonitor class)
    ├── led_manager.h / .cpp         # State → LED pattern mapping
    └── host_protocol.h / .cpp      # USB command/response protocol
```

---

## Key Timing Constants

Define these as named constants — they are expected to need tuning:

```cpp
// state_machine.h
constexpr uint32_t ADAPTER_POLL_INTERVAL_MS      = 1500;  // adapter-present liveness poll
constexpr uint32_t ADAPTER_POLL_INTERVAL_FAST_MS = 100;   // faster poll while watching for an adapter to appear
constexpr uint32_t DUT_INSERT_SETTLE_MS          = 1000;  // suppress DUT polling right after insertion (connector bounce)

// dut_detector.h
constexpr uint32_t DUT_POLL_INTERVAL_MS = 250;
constexpr uint8_t  DUT_CONFIRM_COUNT    = 2;   // consecutive polls required before an insert/remove/flip event fires
```

---

## AT21CS01 EEPROM Layout (36 bytes on the wire: 32-byte header + 4-byte CRC-32)

Packed/unpacked by `eepromSerialize`/`eepromDeserialize` (`src/adapter/eeprom_layout.h/.cpp`):

```
Offset  Size  Field
──────  ────  ──────────────────────────────────────────────────
0       2     magic sentinel
2       1     hwId  (AdapterHardware enum: 0x01=Mezzanine70, 0x02=Mezzanine70r2)
3       1     rfu    (reserved, 0xFF)
4       4     supported_padmap_ids[4]  (0xFF-terminated list; first match against
                                        PadMapRegistry wins — see PadMap Selection Priority)
8       4     reserved
12      4     designed_lifespan   (uint32, max insertions before EOL — set at manufacture)
16      4     date_of_manufacture (uint32, YYYYMMDD)
20      4     insertion_count     (uint32, physical DUT insertions — wear metric)
24      4     test_count          (uint32, completed test runs)
28      4     eol_reached         (uint32: 0 = ok, 0xFFFFFFFF = EOL)
──────────────────────────────────────────────────────────────
32      4     CRC-32 over bytes 0–31
```

Also read separately (not part of this struct): a factory-burned 64-bit serial
number from the AT21CS01's security register (`AT21CS01Driver::readSerial()`),
exposed as the adapter's `aid=` on `ADAPTER`/`EVENT ADAPTER_DETECTED`/`EVENT TEST_START`.

- `insertion_count` — incremented on DUT **absent → present** transition
- `test_count` — incremented on each completed test run (after it completes)
- `eol_reached` — written by firmware when `insertion_count` reaches
  `designed_lifespan`; drives the ID LED on the adapter board and blocks
  further testing (state → `EOL_ADAPTER`)
- If the CRC check fails on read → treat as `CrcError`; a blank (all-0xFF)
  EEPROM is distinguished as `Blank` (`EepromManager::ReadResult`) and reported
  to the host as `ERROR code=7 msg=ADAPTER_NOT_PROVISIONED`, not a generic fault

---

## Adapter Layer

### AdapterBase Interface

```
getDutCount()             → number of DUT slots
selectDut(index)          → activate one slot (no-op for single-DUT adapters)
getPadCount()             → pads per DUT slot
getAdapterHardware()      → hardware ID enum
getSupportedPadmapIds()   → 0xFF-terminated padmap-ID list from EEPROM
channelForPin(adapterPin) → adapter pin (1-indexed) → mux logical channel
selfTest(mux, adc)        → onboard reference-component sanity check, run once per adapter insertion
connectorIsolationSweep(mux, adc, padMap)  → checks for cross-pad shorts on the connector (default: no-op, true)
senseDutPresent(mux, adc, padMap)          → Kelvin sweep on presencePadA/B vs presenceThresholdV
senseDutFlipped(mux, adc, padMap)          → same sweep, mirrored pin pair (adapterPin → 71-adapterPin)
checkDutNow(mux, adc, padMap)              → one-shot presence check (used for the mid/post-test recheck)
setEolLed(bool) / tickEolLed()             → adapter-board EOL indicator (default: no-op)
```

### AdapterRegistry

Maps hardware ID to the correct subclass, instantiated in a static placement-new
buffer (no heap allocation).

```
(0x01)  → Mezzanine70    // 1 DUT; onboard diode used for selfTest()
(0x02)  → Mezzanine70r2  // same board; onboard diode replaced with a 1k precision
                          // resistor, so selfTest() is overridden to check measured
                          // resistance against 1kΩ instead of diode fwd/rev asymmetry
default → nullptr → FAULT (unknown adapter)
```

Everything else about r1/r2 (pad count, channel mapping, presence detection, EOL LED)
is identical — `Mezzanine70r2` subclasses `Mezzanine70` and overrides only
`getAdapterHardware()` and `selfTest()`.

---

## PadMap

Each PadMap describes one die project as a flat list of per-pad test cases —
there is no neighbour-relationship or pad-ring concept; every pad is tested
independently against its own reference pin (`pad_map.h`):

```cpp
struct TestCase {
    uint8_t               adapterPin;  // adapter pin under test (1-indexed)
    uint8_t               gndPin;      // adapter GND pin — reference/return for the sweep
    uint8_t               diePad;      // die pad number (for logging; 0 for DISCHARGE steps)
    TestStrategy          strategy;    // STANDARD | DISCHARGE | CAP_SENSE
    PadType                padType;     // IO | VDDIO | VDD_CORE | PWR_AUX | GND
    uint16_t              settleUs;    // per-case settle time before sampling
    const TestThresholds* thresholds;  // resistance ceiling for GOOD/OPEN (nullptr only valid for DISCHARGE)
};

struct PadMap {
    uint8_t         id;
    const char*     name;
    const TestCase* cases;
    uint8_t         caseCount;
    uint8_t         presencePadA;        // adapter pin pair used for DUT presence/orientation sensing
    uint8_t         presencePadB;        // (Kelvin-swept, not a simple continuity check)
    float           presenceThresholdV;  // sensed voltage below this, at any pullup level → DUT present
};
```

`STANDARD` pads get a Kelvin resistance sweep (330k/33k/3.3k pullup; which
directions are measured is set by `MEASURE_DIRECTIONS` in
`src/test/result.h` — reverse-only in the current build). `CAP_SENSE` pads
(VDDIO/VDD_CORE/PWR_AUX — real bypass caps
that can't settle fast enough at 330k/33k) instead sample a charging curve at
one fixed 3.3k pullup. `DISCHARGE` is a prep step around CAP_SENSE
pads with no result recorded. See `TestRunner` below and `EVENT TEST_START`'s
`max_bond_r_ohms`/`current_list_ua`/`cap_time_list_us` in the host protocol doc
for exactly what's compared against what.

Three pad maps are hardcoded in `PadMapRegistry` today (ids 1–3, all for the
Mezzanine70 board family). Can be extended to receive maps from host via
`SET_PADMAP` later.

### PadMap Selection Priority

1. Host sends `SET_PADMAP id=<n>` command → overrides `_padMap` immediately, any time
2. At adapter init (`StateMachine::selectPadMap`): try each ID in the adapter
   EEPROM's `supported_padmap_ids` list, in order, against `PadMapRegistry::find()`
3. If none match → fall back to `PadMapRegistry::all()`, which returns the
   first registered map (today: id 1) as a default — this is a static
   fallback, not a continuity-based auto-detect

---

## DUT Detector

`DutDetector::poll()` (`src/test/dut_detector.cpp`) runs every `DUT_POLL_INTERVAL_MS`
(250 ms) in all states **except TESTING, NO_ADAPTER, EOL_ADAPTER, FAULT** — driven
from `StateMachine::update()`, not a standalone ticker.

For each poll, delegates to the active `AdapterBase`:
- `senseDutPresent()` — Kelvin sweep (all 3 pullup levels) between `presencePadA`/`presencePadB`;
  present if any level reads below `presenceThresholdV`
- if not present, `senseDutFlipped()` — same sweep on the mirrored pin pair
  (`71 - presencePadA` / `71 - presencePadB`, for a 70-pad Mezzanine board), to
  distinguish "no DUT" from "DUT inserted backwards"

Transitions are debounced: a candidate state (PRESENT / WRONG_ORIENTATION / ABSENT)
must be seen on `DUT_CONFIRM_COUNT` (2) consecutive polls before an event fires,
to absorb connector bounce. `StateMachine` also holds off polling for
`DUT_INSERT_SETTLE_MS` right after an insertion event.

On **absent → present** (`DutEvent::INSERTED`):
1. Increment `insertion_count`, write EEPROM (`flushEeprom()` — also sets `eol_reached` if the limit is hit)
2. Transition to `READY` (or straight to `EOL_ADAPTER` if the limit was just hit)

On **present → absent** (`DutEvent::REMOVED`):
- If not in PASS/FAIL, transition to `ADAPTER_DETECTED`
- In PASS/FAIL: no state change — result LEDs stay lit; a later `RUN` or button
  press re-checks presence via `checkDutNow()` before starting another test
- Either way, runs `connectorIsolationSweep()` afterward to catch cross-pad shorts

On flipped-pin detection (`DutEvent::WRONG_ORIENTATION`): transition to `WRONG_ORIENTATION` state.

---

## Adapter Liveness

There is no separate `AdapterMonitor` class — this is folded into
`StateMachine::update()`/`checkAdapterAlive()`, called every
`ADAPTER_POLL_INTERVAL_MS` (1500 ms) in any state except `NO_ADAPTER`:

```
checkAdapterAlive():
    if eepromMgr.isPresent(): return true
    // adapter physically removed
    clear adapter's EOL LED, send EVENT ADAPTER_REMOVED, drop _adapter/_padMap
    transition → NO_ADAPTER
    return false
```

`isPresent()` is a minimal SWI transaction that returns success/fail without
modifying EEPROM state. In `NO_ADAPTER` state, a separate faster poll
(`ADAPTER_POLL_INTERVAL_FAST_MS`) watches for an adapter appearing, reads its
full EEPROM via `tryInitAdapter()`, instantiates the `AdapterBase` subclass via
`AdapterRegistry`, runs `adapterSelfTest()`, and transitions to
`ADAPTER_DETECTED` (or `EOL_ADAPTER` if already past its insertion limit, or
`FAULT` on EEPROM read/CRC failure).

---

## TestRunner

Adapter-agnostic. Receives an `AdapterBase&` and `PadMap&` (`src/test/test_runner.cpp`).
There is no neighbour concept — every `TestCase` is measured independently
against its own `gndPin`.

```
run(adapter, padMap):
    for slot in 0..adapter.getDutCount()-1:
        adapter.selectDut(slot)
        for tc in padMap.cases:
            if tc.strategy == DISCHARGE:
                short adapterPin + gndPin to Bus::B for tc.settleUs   // drain cap, no result
                continue
            result = sweepPad(adapter, tc)                            // see below
            record result under this slot/channel
        if not dutDetector.checkNow():
            outcome = FAIL_DUT_REMOVED; break                          // DUT pulled mid-test
    // after all slots: PASS only if every tested pad's result is GOOD
    return result
```

`sweepPad(adapter, tc)`:
- **STANDARD** (most IO pads): for each of the 3 pullup levels (330k/33k/3.3k,
  low-current-first) takes one reading per measured direction (forward =
  `adapterPin` driven + Kelvin-sensed with `gndPin` sinking; reverse = roles
  swapped). Which directions are measured is set by `MEASURE_DIRECTIONS`
  (`src/test/result.h`) — reverse-only in the current build, so 3 readings.
  `conducted` = apparent resistance below
  `tc.thresholds->maxBondResistanceOhms` on that reading. Pad is `GOOD` if
  *any* reading conducted (see the rationale for a resistance ceiling over a
  voltage margin in `pad_map.h`).
- **CAP_SENSE** (VDDIO/VDD_CORE/PWR_AUX): single 3.3k pullup, one 5-point
  charging-curve sample set (`measureKelvinCurve`) per measured direction
  instead of one settled reading — classification uses
  only the last (most-settled) sample of each direction.

Apparent resistance: `R = Rpu · V / (VCC − V)`, from the Kelvin voltage read on
COM_A (Bus::A) while the pad under test is driven through one of the pullup
buses (Bus::C/D/E → 330k/33k/3.3k) and the reference pin sinks to Bus::B (tester
GND). COM_D/COM_C are not part of this measurement (see Hardware Summary).

### Result Classification

There is no fixed voltage-threshold table — classification is a single
resistance ceiling (`TestThresholds::maxBondResistanceOhms`, currently 60kΩ,
see the calibration rationale in `pad_map.h`/`pad_map_registry.cpp`), applied
uniformly at every pullup level via `classifyVoltage()` in `hal/kelvin.cpp`:

- Apparent resistance **below the ceiling at any of the measured-direction
  readings (3 reverse per pad for STANDARD, final reverse sample for
  CAP_SENSE in the current build)** → `GOOD`
- No reading below the ceiling → `OPEN`

The forward direction is currently not measured: forward drive charges the
adapter-side bypass cap to ~VCC and is the trigger sequence for the CH446X
latch-up, so the sweep runs reverse-only (`MEASURE_DIRECTIONS =
REVERSE_ONLY` in `src/test/result.h`). The `PAD` line's `method=` field
encodes which directions it carries — `STD`/`STD_FW`/`STD_REV` for STANDARD,
`CAP`/`CAP_FW`/`CAP_REV` for CAP_SENSE, with the corresponding `rf=`/`rr=`/
`vf=`/`vr=`/`vfs=`/`vrs=` groups present only for measured directions — see
`docs/BONDTEST72_HOST_PROTOCOL.md`.

---

## State Machine

```
BOOT ──────────────────────────────────────────────► FAULT
  │ EEPROM ok + self-test ok                          ▲
  ▼                                                   │ mux/ADC error,
NO_ADAPTER ◄────────────────────────────┐             │ unknown adapter
  │ adapter detected                   │             │
  ▼                                    │ adapter     │
ADAPTER_DETECTED                       │ removed     │
  │ DUT detected                       │             │
  ▼                                    │             │
READY ──── button pressed ──► TESTING ─┤             │
  ▲                               │    │             │
  │ new DUT detected              │    └─────────────┘
  │ (from PASS or FAIL)           ▼
  │                          PASS / FAIL
  └──────────────────────────────┘
         (result LEDs stay lit after DUT removed;
          cleared only when new DUT is detected)
```

---

## LED Behaviour (3× SK6812, positions 0–2)

| State | LED 0 — Ready | LED 1 — Pass | LED 2 — Fail |
|-------|--------------|-------------|-------------|
| NO_ADAPTER | off | off | off |
| ADAPTER_DETECTED | yellow slow blink | off | off |
| READY | yellow solid | off | off |
| TESTING | yellow fast blink | off | off |
| PASS | off | green solid | off |
| FAIL | off | off | red solid |
| FAULT | off | off | red fast blink |

Result LEDs (PASS / FAIL) remain lit while DUT is absent. Cleared when the next DUT
is detected, transitioning back to READY.

---

## Host Protocol (USB CDC)

Line-oriented text protocol using `key=value` pairs for all messages with arguments. 115200 baud.

### Commands (host → tester)

```
RUN                              trigger test (same as button press)
GET_ADAPTER                      query adapter info
GET_RESULTS                      re-send last result set
SET_PADMAP id=<uint>             select active pad map by project ID
PROVISION hw=<uint> padmap=<uint>[,<uint>...] lifespan=<uint> date=<YYYYMMDD>  write adapter EEPROM (factory use)
DISCOVERY_SCAN                   sweep all pad pairs, stream sense voltages
```

### Events and responses (tester → host)

```
EVENT ADAPTER_DETECTED aid=<hex16> ahw=<uint> pm=<uint>[,<uint>...]
EVENT ADAPTER_REMOVED
EVENT DUT_INSERTED
EVENT DUT_REMOVED
EVENT TEST_START aid=<hex16> ahw=<uint> ins=<uint> tests=<uint> pm=<uint>[,<uint>...] current_list_ua=<f,f,f> [max_bond_r_ohms=<float>] [cap_time_list_us=<uint,...>]
EVENT EOL_WARNING ins=<uint>
EVENT WRONG_ORIENTATION
EVENT FAULT msg=<string>

ADAPTER aid=<hex16> ahw=<uint> pm=<uint>[,<uint>...] lifespan=<uint> mfg_date=<YYYYMMDD> ins=<uint> tests=<uint> eol=<0|1> dut=<0|1>

SLOT slot=<uint> present=<0|1> tested=<0|1>          # sent before that slot's PAD lines

PAD slot=<uint> apin=<uint> dp=<uint> method=STD_REV result=<GOOD|OPEN> rr=<f,f,f> vr=<f,f,f>
PAD slot=<uint> apin=<uint> dp=<uint> method=CAP_REV result=<GOOD|OPEN> vrs=<f,f,f,f,f>

SUMMARY outcome=<PASS|FAIL> good=<uint> tested=<uint> [fail_reason=DUT_REMOVED]

DSCAN src=<uint> snk=<uint> sv=<float>
DSCAN DONE

OK PROVISION

ERROR code=<uint> msg=<string>
```

### Error codes

| Code | Name | Description |
|------|------|-------------|
| 1 | NO_ADAPTER | No adapter detected |
| 2 | BUSY | Tester busy (testing or scanning) |
| 3 | UNKNOWN_PADMAP | Pad map ID not found |
| 4 | PROVISION_FAILED | EEPROM write failed |
| 5 | NOT_IMPLEMENTED | Command not implemented |
| 6 | MISSING_FIELD | Required PROVISION field omitted |
| 7 | ADAPTER_NOT_PROVISIONED | Adapter EEPROM present but blank |

---

## Mid-Test DUT Removal

DUT polling is **suspended during TESTING** to avoid MUX contention.

If the DUT is removed mid-test, the ESD diode path disappears and remaining pads read
as open (~3.3 V) — they naturally register as FAIL. At the end of the scan,
`TestRunner` performs one final `dutDetector.checkNow()`:
- DUT absent → `result.cause = FAIL_DUT_REMOVED`
- Result is sent to host; state machine transitions to FAIL

---

## Deferred / Future Work

| Item | Notes |
|------|-------|
| External ADC reference | Optional external Vref for Pico 2 ADC — not needed for MVP |
| PadMap over USB | `SET_PADMAP` with full map payload (for new die projects without firmware update) |
| EOL service limit values | Per-model thresholds TBD; placeholders in AdapterRegistry |

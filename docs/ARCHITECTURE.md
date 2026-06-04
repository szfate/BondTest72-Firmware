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
| ADC | Internal RP2350 ADC; reads COM_D (ADC0/GP26), COM_A (ADC1/GP27), COM_C (ADC2/GP28) |
| USB | CDC serial to host PC |
| Adapter connector | 2×41 pin (82 pins): 72 measurement channels + 7 adapter-control GPIOs + power/GND |

The last pin of CON6 (adapter header) is dedicated to the AT21CS01 SWI data line.

---

## Layer Diagram

```
┌──────────────────────────────────────────────────────────┐
│                    Application Layer                     │
│     StateMachine  ·  HostProtocol  ·  AdapterMonitor    │
├──────────────────────────┬───────────────────────────────┤
│       Test Engine        │       Adapter Layer           │
│  TestRunner  PadMap      │  AdapterBase (interface)      │
│  PadMapRegistry          │  Mezzanine70                  │
│  DutDetector  Discovery  │  Mezzanine70x5 (future)       │
│  ResultStore             │  AdapterRegistry              │
├──────────────────────────┴───────────────────────────────┤
│               Hardware Abstraction Layer (HAL)           │
│   MuxController · ADC · SK6812Controller                 │
│   Buttons · AT21CS01Driver · UsbSerial                   │
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
│   ├── mux_map.h                    # MUX_MAP[72]: logical pad → (chip, channel); filled from schematic
│   ├── adc.h / adc.cpp              # ADC wrapper: readVoltage()
│   ├── sk6812.h / sk6812.cpp        # addressable LED driver, 3 pixels, single GPIO
│   ├── buttons.h / buttons.cpp      # debounced start button input
│   ├── at21cs01.h / at21cs01.cpp    # Microchip SWI EEPROM driver (NOT Dallas 1-Wire)
│   └── usb_serial.h / usb_serial.cpp
│
├── adapter/
│   ├── adapter_base.h               # pure abstract interface
│   ├── eeprom_layout.h              # EepromData struct + serialize/deserialize
│   ├── mezzanine70.h / .cpp         # 1-DUT adapter, no isolation switches
│   ├── mezzanine70x5.h / .cpp       # 5-DUT adapter, drives isolation GPIOs (future)
│   └── adapter_registry.h / .cpp   # (model, version) → AdapterBase* factory
│
├── test/
│   ├── pad_map.h / pad_map.cpp             # pad roles + presence detection recipe
│   ├── pad_map_registry.h / .cpp           # 4 hardcoded PadMap instances
│   ├── dut_detector.h / dut_detector.cpp  # continuity polling via PadMap recipe
│   ├── test_runner.h / test_runner.cpp     # test orchestration, adapter-agnostic
│   ├── discovery.h / discovery.cpp         # discovery scan (unknown pad map)
│   └── result.h                            # result structs (per-pad, per-DUT)
│
└── app/
    ├── state_machine.h / .cpp       # top-level states, drives LEDs, handles buttons
    ├── adapter_monitor.h / .cpp     # periodic EEPROM ping for hot-removal detection
    └── host_protocol.h / .cpp      # USB command/response protocol
```

---

## Key Timing Constants

Define these as named constants — they are expected to need tuning:

```cpp
// adapter_monitor.h
constexpr uint32_t ADAPTER_POLL_INTERVAL_MS = 2000;

// dut_detector.h
constexpr uint32_t DUT_POLL_INTERVAL_MS = 250;
```

---

## AT21CS01 EEPROM Layout (128 bytes)

```
Offset  Size  Field
──────  ────  ──────────────────────────────────────────────────
0       2     magic: { 0xB7, 0x72 }  — "BT72" sentinel
2       1     adapter_model      (enum: 0x01 = Mezzanine70, ...)
3       1     adapter_version    (e.g. 0x01)
4       1     supported_padmap_id  (0xFF = unset → auto-detect)
5       1     reserved
6       2     designed_lifespan  (uint16, max insertions before EOL — set at manufacture)
8       4     date_of_manufacture  (YYYYMMDD)
12      4     insertion_count  (physical DUT insertions — wear metric)
16      4     test_count       (completed test runs)
20      1     eol_reached                (0x00 = ok, 0xFF = EOL)
21      1     reserved
22      2     CRC-16 over bytes 0–21
──────────────────────────────────────────────────────────────
24–127  104   reserved / future use
```

- `insertion_count` — incremented on DUT **absent → present** transition
- `test_count` — incremented on each completed test run
- `eol_reached` — written by firmware when `insertion_count` exceeds the
  service limit for the adapter model; drives the ID LED on the adapter board
- If CRC check fails on read → treat as FAULT (bad EEPROM or no adapter)

---

## Adapter Layer

### AdapterBase Interface

```
getDutCount()          → number of DUT slots (1 or 5)
selectDut(index)       → activate one slot (drives isolation GPIOs; no-op for single-DUT)
getPadCount()          → pads per DUT slot
getAdapterModel()      → model enum
getAdapterVersion()    → version byte
getSupportedPadmapId() → padmap hint from EEPROM (or 0xFF)
```

### AdapterRegistry

Maps (model, version) to the correct subclass. DUT count is encoded in the subclass,
not stored in EEPROM.

```
(0x01, *)  → Mezzanine70     // 1 DUT, no isolation switches
(0x02, *)  → Mezzanine70x5   // 5 DUTs, drives isolation GPIOs (future)
default    → nullptr → FAULT (unknown adapter)
```

---

## PadMap

Each PadMap describes one die project and includes a presence detection recipe:

```cpp
struct PadMap {
    ProjectId   id;
    const char* name;
    PadRole     roles[72];              // IO, GND, VCC, NC per pad index
    uint8_t     gndPad;                 // current injection pad (COM_A)
    uint8_t     ioPadsInRingOrder[72];  // IO pads in physical pad-ring order; defines neighbour relationships
    uint8_t     ioPadCount;             // number of entries in ioPadsInRingOrder
    uint8_t     presencePadA;           // } two pads shorted through
    uint8_t     presencePadB;           // } the DUT PCB — continuity = DUT present
    float       presenceThresholdV;     // COM_A reading below this → DUT present (e.g. 0.3 V)
};
```

Four pad maps are hardcoded in `PadMapRegistry` to start. Can be extended to receive
maps from host via `SET_PADMAP` command later.

### PadMap Selection Priority

1. Host sends `SET_PADMAP <id>` command → use that
2. Adapter EEPROM `supported_padmap_id` is set (≠ 0xFF) → look up in registry
3. Neither → auto-detect: try each PadMap's presence recipe until one shows continuity

---

## DUT Detector

`DutDetector::poll()` runs every `DUT_POLL_INTERVAL_MS` in all states **except TESTING**.

For each poll:
- Routes `presencePadA` to BUS_C, `presencePadB` to BUS_D (separate from test buses)
- Injects small current; reads ADC
- `voltage < presenceThresholdV` → DUT present

On **absent → present** transition:
1. Increment `insertion_count`, write EEPROM
2. Notify StateMachine: `DUT_INSERTED`

On **present → absent** transition during PASS/FAIL state:
- No state change — result LED stays lit
- Notify StateMachine: `DUT_REMOVED` (used for logging only in these states)

New DUT detection (absent → present) in PASS/FAIL state:
- Clear result LEDs
- Transition to READY

---

## Adapter Monitor

`AdapterMonitor::tick()` fires every `ADAPTER_POLL_INTERVAL_MS` (2000 ms).
Also fires immediately on START button press (belt-and-suspenders check before test).

```
ping EEPROM:
    fail  → StateMachine: ADAPTER_REMOVED → NO_ADAPTER state, inhibit button
    ok    → if previously in NO_ADAPTER: read full EEPROM, instantiate adapter,
             StateMachine: ADAPTER_DETECTED
```

Ping = minimal SWI transaction that returns success/fail without modifying EEPROM state.

---

## TestRunner

Adapter-agnostic. Receives an `AdapterBase*` and `PadMap*`.

```
run(adapter, padMap):
    for slot in 0..adapter.getDutCount()-1:
        adapter.selectDut(slot)
        for i, ioPad in enumerate(padMap.ioPadsInRingOrder):
            leftNeighbour  = ioPadsInRingOrder[(i - 1 + count) % count]
            rightNeighbour = ioPadsInRingOrder[(i + 1) % count]
            mux.setChannel(padMap.gndPad, BUS_D)   // inject current — COM_D sensed by ADC (ADC0)
            mux.setChannel(ioPad,         BUS_B)   // return path (tester GND)
            mux.setChannel(leftNeighbour, BUS_A)   // neighbour sense — COM_A sensed by ADC (ADC1)
            mux.setChannel(rightNeighbour,BUS_C)   // neighbour sense — COM_C sensed by ADC (ADC2)
            readings = adc.readAll()               // returns {comD, comA, comC}
            results[slot][ioPad] = classify(readings)
        presence = dutDetector.checkNow()
        if presence == ABSENT:
            results[slot].cause = FAIL_DUT_REMOVED
    mux.clearAll()
    return results
```

### ADC Channels and Bias Circuits

| Bus | COM pin | GPIO | ADC ch | Bias circuit | "Normal" reading | Short reading |
|-----|---------|------|--------|-------------|-----------------|---------------|
| BUS_D | COM_D | GP26 | ADC0 | 27 kΩ pullup to 3.3 V | 0.5–0.7 V (good bond) | ~0 V |
| BUS_A | COM_A | GP27 | ADC1 | 1 MΩ/220 kΩ divider (3.3 V) | ~0.6 V | ~0 V |
| BUS_C | COM_C | GP28 | ADC2 | 1 MΩ/220 kΩ divider (3.3 V) | ~0.6 V | ~0 V |

COM_D carries the injection+sense bus (27 kΩ pullup). COM_A and COM_C carry the
neighbour sense buses (1 MΩ/220 kΩ divider, ~0.6 V idle). BUS_B is tester GND — no ADC.
BUS_E is spare. GP29 (ADC3) is internal VSYS monitor — not available on header.

### Result Classification

COM_D (injection+sense):
- `0.5–0.7 V` → **GOOD** bond
- `> ~2.5 V`  → **OPEN** bond (no return path, pulled to 3.3 V by pullup)
- `< ~0.1 V`  → **SHORT** to GND

COM_A / COM_C (neighbour sense):
- `~0.6 V`    → no short to neighbour
- `< ~0.2 V`  → **SHORT** between bond-under-test and this neighbour

Thresholds defined per PadMap (process-dependent).

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
PROVISION padmap=<uint> [date=<YYYYMMDD>]  write adapter EEPROM (factory use)
DISCOVERY_SCAN                   sweep all pad pairs, stream sense voltages
```

### Events and responses (tester → host)

```
EVENT ADAPTER_DETECTED uid=<hex16> model=<uint> ver=<uint> pm=<uint> [<pm=<uint>> ...]
EVENT ADAPTER_REMOVED
EVENT DUT_INSERTED
EVENT DUT_REMOVED
EVENT TEST_START uid=<hex16> model=<uint> ver=<uint> pm=<uint> [<pm=<uint>> ...]
EVENT EOL_WARNING ins=<uint>
EVENT WRONG_ORIENTATION
EVENT FAULT msg=<string>

ADAPTER uid=<hex16> model=<uint> ver=<uint> padmap0=<uint> [padmap1=<uint> ...] lifespan=<uint> mfg_date=<YYYYMMDD> ins=<uint> tests=<uint> eol=<0|1> padmap=<name|none>

PAD slot=<uint> mez=<uint> dp=<uint> result=<GOOD|OPEN|SHORT> ps=<0|1> ns=<0|1> sv=<float> pv=<float> nv=<float>

SLOT slot=<uint> present=<0|1> tested=<0|1>

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
| Mezzanine70x5 adapter | 5-DUT, isolation GPIO mapping TBD when hardware exists |
| Discovery scan | Iterates all pad combinations to identify GND pad on unknown die |
| Binary host protocol | Swap text protocol for framed binary once PC software matures |
| Computer vision integration | `EVENT TEST_START` is the hook; CV side is future work |
| External ADC reference | Optional external Vref for Pico 2 ADC — not needed for MVP |
| PadMap over USB | `SET_PADMAP` with full map payload (for new die projects without firmware update) |
| EOL service limit values | Per-model thresholds TBD; placeholders in AdapterRegistry |
| Multi-slot READY logic | For Mezzanine70x5: whether all 5 slots must be loaded or any 1 is enough |

# BondTest72 Firmware

Firmware for the BondTest72 wirebond integrity tester — a bench instrument for testing bare IC die (up to 72 pads) from [wafer.space](https://wafer.space) runs.

---

## How it works

The tester measures each die pad with a Kelvin resistance sweep. The pad under test is driven through one of three pullup strengths (330 kΩ / 33 kΩ / 3.3 kΩ — 10/100/1000 µA, weakest current first) against the pad map's GND reference pin, and the driven node is Kelvin-sensed on COM_A. The apparent bond resistance `R = Rpu · V / (VCC − V)` is computed for each reading; a pad is **GOOD** if any reading falls below the resistance ceiling (60 kΩ), **OPEN** if none do — there is no fixed voltage-threshold table.

```
                        3.3 V
                          │
                 330k / 33k / 3.3k   ← pullup sweep, weakest current first
                          │
       driven pin ────────┤ ← Kelvin sense (COM_A, ADC1)
                          │
                   path under test
             (bond · die trace · ESD structure)
                          │
      reference pin ──────┴──→ tester GND return (Bus::B)
```

The sweep runs **reverse-only** in the current build (`MEASURE_DIRECTIONS = REVERSE_ONLY` in `src/test/result.h`): the forward direction charges the adapter-side bypass cap to ~VCC and is the trigger sequence for the CH446X latch-up (see `src/hal/kelvin.cpp`). Forward is kept available for root-cause isolation.

### Test strategies

Each pad map is a flat list of per-pad test cases — every pad is measured independently against its own reference pin (there is no neighbour-ring concept):

1. **STANDARD** — most IO pads. One reading per pullup level per measured direction (3 reverse readings today).
2. **CAP_SENSE** — pads with a real bypass/decoupling cap (VDDIO/VDD_CORE/PWR_AUX) that can't settle at 330k/33k (τ = R·C). Uses only the 3.3k level and samples a 5-point charging curve; classification uses the final (most-settled) sample.
3. **DISCHARGE** — prep step around CAP_SENSE pads: shorts pad + return to GND to drain the cap, no result recorded.

Before every reading the pad and its return are grounded to a known baseline, and the driven node is drained again before the mux releases — the order is safety-critical and doubles as the CH446X latch-up mitigation (`groundAndDischarge`/`drainAndRelease` in `src/hal/kelvin.cpp`).

---

## Hardware

The system is a two-board design:

```
┌──────────────────────────────────┐     ┌──────────────────────────┐
│         Tester Board             │     │       Adapter Board      │
│                                  │     │                          │
│  RP2350 (Raspberry Pi Pico 2)    │     │  DUT connector (70-pin)  │
│  3× CH446X matrix analog MUX ───────── │  AT21CS01 EEPROM         │
│  3× SK6812 RGB LED               │     │  (carries pad map ID)    │
│  Start button                    │     │                          │
│  USB CDC to host PC              │     └──────────────────────────┘
└──────────────────────────────────┘
```

The adapter board is specific to a die form factor (e.g. 1×0.5, 1×1) and connector type. Different adapters can also support testing customer boards directly, provided a compatible connector is used. The adapter's EEPROM stores metadata (hardware ID, pad map ID, insertion count, manufacturing date) so the firmware automatically selects the correct test sequence when an adapter is plugged in.

| Component | Part | Notes |
|-----------|------|-------|
| MCU | RP2350 (Raspberry Pi Pico 2) | Dual-core Cortex-M33, 520 kB SRAM |
| Muxes | 3× CH446X 24:5 matrix | 72 channels total |
| LEDs | 3× SK6812 addressable RGB | Single-GPIO series chain |
| EEPROM | AT21CS01 | Microchip Single-Wire Interface (SWI) — **not** Dallas/Maxim 1-Wire |
| Host interface | USB CDC serial | 115200 baud, line-oriented text |

### ADC buses

| Bus | GPIO | ADC | Role |
|-----|------|-----|------|
| COM_D | GP26 | ADC0 | Not sampled by the test path — available for debug tooling |
| COM_A | GP27 | ADC1 | Kelvin sense — the production measurement input |
| COM_C | GP28 | ADC2 | Not sampled by the test path — available for debug tooling |

### Known adapters

| Adapter | Sticker | Pad map ID | Die | Notes |
|---------|---------|-----------|-----|-------|
| Mezzanine70 r1 | COB v1  | 1 | 1x1   | Early die samples with an unconnected trace |
| Mezzanine70 r1 | COB v2+ | 2 | 1x1   | Fixed COB boards |
| Mezzanine70 r1 | 1x0p5       | 3 | 1x0p5 | First 1x0p5 die adapter |

Hardware IDs (`ahw` / EEPROM byte 2): `0x01` = Mezzanine70 r1, `0x02` = Mezzanine70 r2. The r2 board is identical except the onboard self-test diode is replaced with a 1 kΩ precision resistor, so `selfTest()` checks measured resistance instead of diode asymmetry.

---

## Firmware Architecture

```
┌──────────────────────────────────────────────────────────┐
│                    Application Layer                     │
│      StateMachine  ·  HostProtocol  ·  LedManager        │
├──────────────────────────┬───────────────────────────────┤
│       Test Engine        │       Adapter Layer           │
│  TestRunner  PadMap      │  AdapterBase (interface)      │
│  PadMapRegistry          │  Mezzanine70 / Mezzanine70r2  │
│  DutDetector             │  AdapterRegistry              │
│  DiscoveryScanner        │  EepromManager                │
├──────────────────────────┴───────────────────────────────┤
│               Hardware Abstraction Layer (HAL)           │
│   MuxController · AdcDriver · Kelvin (measurement)       │
│   SK6812Controller · Buttons · AT21CS01Driver            │
├──────────────────────────────────────────────────────────┤
│                    RP2350 / Arduino                      │
└──────────────────────────────────────────────────────────┘
```

Adapter liveness and DUT polling are folded into `StateMachine::update()` — there is no separate AdapterMonitor class.

### State machine

```
BOOT ──────────────────────────────────────────────► FAULT
  │ self-test ok                                       ▲
  ▼                                                    │ mux/ADC error,
NO_ADAPTER ◄──────────────────────────┐               │ unknown adapter,
  │ adapter detected                  │               │ blank EEPROM
  ▼                                   │ adapter       │
ADAPTER_DETECTED ──EOL reached──► EOL_ADAPTER          │
  │ DUT detected                      │ removed       │
  ▼                                   │               │
READY ──── button / RUN cmd ──► TESTING ──────────────┘
  ▲  \                              │
  │   \─ DUT inserted backwards ─► WRONG_ORIENTATION
  │ new DUT detected                │
  │ (clears result LEDs)            ▼
  └──────────────────────────  PASS / FAIL
```

Result LEDs stay lit after the DUT is removed. They clear only when the next DUT is detected.

### LED states

| State | LED 0 — Ready | LED 1 — Pass | LED 2 — Fail |
|-------|--------------|-------------|-------------|
| No adapter / EOL adapter | dim red | dim red | dim red |
| Adapter detected | yellow slow blink | off | off |
| Ready | yellow solid | off | off |
| Testing | yellow solid | off | off |
| Wrong orientation | red slow blink | off | off |
| Pass | off | green solid | off |
| Fail | off | off | red solid |
| Fault | off | off | red fast blink |

---

## Building and Flashing

Built with [PlatformIO](https://platformio.org/) using the `earlephilhower/arduino-pico` core.

```bash
pio run                            # build
pio run -t upload                  # build and flash (via picotool)
pio device monitor --baud 115200   # open serial monitor
```

Hold the BOOTSEL button on the Pico 2 while connecting USB to enter the bootloader if picotool cannot find the device.

---

## Host Protocol

Line-oriented text over USB CDC, 115200 baud. All messages with arguments use `key=value` pairs. Commands are sent from host to tester; events and results are streamed back.

### Commands (host → tester)

| Command | Description |
|---------|-------------|
| `HELLO` | Request firmware name, build ID, and board UID |
| `RUN` | Trigger a test (same as pressing the button) |
| `GET_RESULTS` | Re-send the last result set |
| `GET_ADAPTER` | Query current adapter info |
| `SET_PADMAP id=<uint>` | Override pad map selection |
| `PROVISION hw=<uint> padmap=<uint>[,<uint>...] lifespan=<uint> date=<YYYYMMDD> [ins=<uint>] [tests=<uint>] [eol=<0\|1>]` | Write adapter EEPROM (factory use). Counters default to 0 when omitted |
| `DISCOVERY_SCAN` | Sweep all 70×70 pad pairs, stream sense voltages |

### Responses (tester → host)

```
HELLO name=<string> build=<string> uid=<hex16>

ADAPTER aid=<hex16> ahw=<uint> pm=<uint>[,<uint>...] lifespan=<uint> mfg_date=<YYYYMMDD> ins=<uint> tests=<uint> eol=<0|1> dut=<0|1>

SLOT slot=<uint> present=<0|1> tested=<0|1>          # sent before that slot's PAD lines

PAD slot=<uint> apin=<uint> dp=<uint> method=STD_REV result=<GOOD|OPEN> rr=<f,f,f> vr=<f,f,f>
PAD slot=<uint> apin=<uint> dp=<uint> method=CAP_REV result=<GOOD|OPEN> vrs=<f,f,f,f,f>

# method= encodes which direction group(s) the line carries:
#   STD / CAP       both directions (rf= + rr= / vfs= + vrs=)
#   STD_FW / CAP_FW forward only (rf= / vfs=)
#   STD_REV / CAP_REV reverse only (rr= / vrs=)
# The current build is reverse-only, so lines carry STD_REV / CAP_REV and
# only the reverse groups. Unmeasured groups are never sent as zeros.

SUMMARY outcome=<PASS|FAIL> good=<uint> tested=<uint> [fail_reason=DUT_REMOVED]

EVENT ADAPTER_DETECTED aid=<hex16> ahw=<uint> pm=<uint>[,<uint>...]
EVENT ADAPTER_REMOVED
EVENT DUT_INSERTED
EVENT DUT_REMOVED
EVENT TEST_START aid=<hex16> ahw=<uint> ins=<uint> tests=<uint> pm=<uint>[,<uint>...] current_list_ua=<f,f,f> [max_bond_r_ohms=<float>] [cap_time_list_us=<uint,...>]
EVENT EOL_WARNING ins=<uint>
EVENT WRONG_ORIENTATION
EVENT FAULT msg=<string>

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

## Adapter EEPROM

Each adapter carries an AT21CS01 EEPROM that stores lifetime and configuration data. The firmware reads this on adapter insertion and writes back counters after each test.

| Offset | Size | Field |
|--------|------|-------|
| 0 | 2 | Magic: `{ 0xB7, 0x72 }` |
| 2 | 1 | `adapter_hardware` (0x01 = Mezzanine70 r1, 0x02 = Mezzanine70 r2) |
| 3 | 1 | `rfu` (reserved, write 0xFF) |
| 4 | 4 | `supported_padmap_ids` (0xFF-terminated list, up to 4 IDs; first match against the pad map registry wins) |
| 8 | 4 | reserved (all zeros) |
| 12 | 4 | `designed_lifespan` — max insertions before EOL |
| 16 | 4 | `date_of_manufacture` — YYYYMMDD as uint32 (e.g. 20260101) |
| 20 | 4 | `insertion_count` — DUT insertions (wear metric) |
| 24 | 4 | `test_count` — completed test runs |
| 28 | 4 | `eol_reached` (0x00000000 = ok, 0xFFFFFFFF = EOL) |
| 32 | 4 | CRC-32 over bytes 0–31 |

`insertion_count` increments on each DUT absent → present transition. When it exceeds the `designed_lifespan` for the adapter model the firmware sets `eol_reached` and drives the ID LED on the adapter board.

---

## Tools

Host-side Python scripts for interacting with the tester. See [tools/README.md](tools/README.md) for full usage.

---

## Repository Layout

```
src/
├── main.cpp              entry point — constructs and wires all layers
├── hal/                  hardware drivers (MUX, Kelvin measurement, ADC, LEDs, buttons, EEPROM)
├── adapter/              adapter abstraction, EEPROM layout/manager, Mezzanine70 drivers
├── test/                 test runner, pad maps, DUT detector, discovery scan
├── app/                  state machine, LED manager, host protocol
└── debug/                onboard self-tests (mux waveform, ADC, EEPROM, SK6812, buttons)
docs/
├── ARCHITECTURE.md       detailed firmware design notes
├── BONDTEST72_HOST_PROTOCOL.md  full host protocol reference
├── ADAPTER_MEZ70.md      Mezzanine70 adapter pin map and behaviour
├── MUX_MAP.md            logical pad → MUX chip/channel mapping
├── DUT_PADMAP_TEMPLATE.md  pad map template for new adapters
├── DUT_PADMAP_1X1.md       pad map — 1x1 die (Mezzanine70)
├── DUT_PADMAP_1X0P5.md     pad map — 1x0p5 die
├── GLOSSARY.md           shared vocabulary
└── RP2350 PINMAP.md      MCU GPIO assignments
tools/                    host-side Python scripts
```

See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for the full firmware design reference.

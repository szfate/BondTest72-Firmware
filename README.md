# BondTest72 Firmware

Firmware for the BondTest72 wirebond integrity tester — a bench instrument for testing bare IC die (up to 72 pads) from [wafer.space](https://wafer.space) runs.

---

## How it works

The tester injects a small constant current (~100 µA) through the GND pad of the die and measures the resulting voltage at each IO pad. Because every IO pad has an ESD protection diode tied to GND internally, a good bond produces a predictable forward-drop voltage. Missing or shorted bonds deviate from this signature.

```
                         3.3 V
                           │
                        27 kΩ  ← pullup on COM_D
                           │
          ┌────────────────┤ COM_D (ADC0) ← bond sense
          │                │
  GND pad ┤ ~100 µA →  ESD diode
          │                │
          └────────────────┤ IO pad under test
```

| COM_D voltage | Interpretation |
|---------------|----------------|
| 0.2 – 0.8 V   | Good bond (ESD diode forward drop) |
| > 2.5 V       | Open bond — no return path, pullup dominates |
| < 0.1 V       | Short to GND |

### Two-phase bond testing

Each pad is tested in two phases to catch both bond defects and inter-pad shorts:

1. **Neighbour phase** — the pad under test is grounded and its immediate ring-neighbours are sensed via a 1 MΩ/220 kΩ divider (~0.6 V idle). A low reading on a neighbour indicates an inter-pad short.
2. **Injection phase** — current is injected via the GND pad and the bond voltage is measured on COM_D. Neighbour sense is not active during this phase to avoid interaction.

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

| Bus | GPIO | ADC | Bias circuit | Idle reading |
|-----|------|-----|-------------|-------------|
| COM_D — bond sense | GP26 | ADC0 | 27 kΩ pullup to 3.3 V | ~3.3 V (no DUT) |
| COM_A — left neighbour sense | GP27 | ADC1 | 1 MΩ/220 kΩ divider | ~0.6 V |
| COM_C — right neighbour sense | GP28 | ADC2 | 1 MΩ/220 kΩ divider | ~0.6 V |

### Known adapters

| Adapter | Sticker | Pad map ID | Die | Notes |
|---------|---------|-----------|-----|-------|
| Mezzanine70 r1 | COB v1  | 1 | 1x1   | Early die samples with an unconnected trace |
| Mezzanine70 r1 | COB v2+ | 2 | 1x1   | Fixed COB boards |
| Mezzanine70 r1 | 1x0p5       | 3 | 1x0p5 | First 1x0p5 die adapter |

---

## Firmware Architecture

```
┌──────────────────────────────────────────────────────────┐
│                    Application Layer                     │
│     StateMachine  ·  HostProtocol  ·  AdapterMonitor     │
├──────────────────────────┬───────────────────────────────┤
│       Test Engine        │       Adapter Layer           │
│  TestRunner  PadMap      │  AdapterBase (interface)      │
│  PadMapRegistry          │  Mezzanine70                  │
│  DutDetector  Discovery  │  AdapterRegistry              │
├──────────────────────────┴───────────────────────────────┤
│               Hardware Abstraction Layer (HAL)           │
│   MuxController · ADC · SK6812Controller                 │
│   Buttons · AT21CS01Driver · UsbSerial                   │
├──────────────────────────────────────────────────────────┤
│                    RP2350 / Arduino                      │
└──────────────────────────────────────────────────────────┘
```

### State machine

```
BOOT ──────────────────────────────────────────────► FAULT
  │ self-test ok                                       ▲
  ▼                                                    │ mux/ADC error,
NO_ADAPTER ◄──────────────────────────┐               │ unknown adapter
  │ adapter detected                  │               │
  ▼                                   │ adapter       │
ADAPTER_DETECTED                      │ removed       │
  │ DUT detected                      │               │
  ▼                                   │               │
READY ──── button / RUN cmd ──► TESTING ──────────────┘
  ▲                                 │
  │ new DUT detected                │
  │ (clears result LEDs)            ▼
  └──────────────────────────  PASS / FAIL
```

Result LEDs stay lit after the DUT is removed. They clear only when the next DUT is detected.

### LED states

| State | LED 0 — Ready | LED 1 — Pass | LED 2 — Fail |
|-------|--------------|-------------|-------------|
| No adapter | off | off | off |
| Adapter detected | yellow slow blink | off | off |
| Ready | yellow solid | off | off |
| Testing | yellow fast blink | off | off |
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
| `PROVISION padmap=<uint> [date=<YYYYMMDD>]` | Write adapter EEPROM (factory use) |
| `DISCOVERY_SCAN` | Sweep all 70×70 pad pairs, stream sense voltages |

### Responses (tester → host)

```
HELLO name=<string> build=<string> uid=<hex16>

ADAPTER uid=<hex16> hw=<uint> pm=<uint>[,<uint>...] lifespan=<uint> mfg_date=<YYYYMMDD> ins=<uint> tests=<uint> eol=<0|1>

PAD slot=<uint> apin=<uint> dp=<uint> result=<GOOD|OPEN|SHORT> ps=<0|1> ns=<0|1> sv=<float> pv=<float> nv=<float>

SLOT slot=<uint> present=<0|1> tested=<0|1>

SUMMARY outcome=<PASS|FAIL> good=<uint> tested=<uint> [fail_reason=DUT_REMOVED]

EVENT ADAPTER_DETECTED uid=<hex16> hw=<uint> pm=<uint>[,<uint>...]
EVENT ADAPTER_REMOVED
EVENT DUT_INSERTED
EVENT DUT_REMOVED
EVENT TEST_START uid=<hex16> hw=<uint> pm=<uint>[,<uint>...]
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

---

## Adapter EEPROM

Each adapter carries an AT21CS01 EEPROM that stores lifetime and configuration data. The firmware reads this on adapter insertion and writes back counters after each test.

| Offset | Size | Field |
|--------|------|-------|
| 0 | 2 | Magic: `{ 0xB7, 0x72 }` |
| 2 | 1 | `adapter_hardware` (0x01 = Mezzanine70) |
| 3 | 1 | `rfu` (reserved, write 0xFF) |
| 4 | 4 | `supported_padmap_ids` (0xFF = unused slot, up to 4 IDs) |
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
├── hal/                  hardware drivers (MUX, ADC, LEDs, buttons, EEPROM, USB)
├── adapter/              adapter abstraction, EEPROM layout, Mezzanine70 driver
├── test/                 test runner, pad maps, DUT detector, discovery scan
└── app/                  state machine, adapter monitor, host protocol
docs/
├── ARCHITECTURE.md       detailed firmware design notes
├── MUX_MAP.md            logical pad → MUX chip/channel mapping
├── DUT_PADMAP_TEMPLATE.md  pad map template for new adapters
├── DUT_PADMAP_1X1.md       pad map — 1x1 die (Mezzanine70)
├── DUT_PADMAP_1X0P5.md     pad map — 1x0p5 die
└── RP2350 PINMAP.md      MCU GPIO assignments
tools/                    host-side Python scripts
```

See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for the full firmware design reference.

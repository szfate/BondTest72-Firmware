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

The adapter board is specific to a die form factor (e.g. 1×0.5 mm, 0.5×1 mm, 0.5×0.5 mm) and connector type. Different adapters can also support testing customer boards directly, provided a compatible connector is used. The adapter's EEPROM stores metadata (adapter model, version, pad map ID, insertion count, manufacturing date) so the firmware automatically selects the correct test sequence when an adapter is plugged in.

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
| Mezzanine70 r1 | 1x0p5       | 3 | 1x0p5 | First 1x0.5 mm die adapter |

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

Line-oriented text over USB CDC, 115200 baud. Commands are sent from host to tester; events and results are streamed back.

### Commands (host → tester)

| Command | Description |
|---------|-------------|
| `RUN` | Trigger a test (same as pressing the button) |
| `GET_RESULTS` | Re-send the last result set |
| `GET_ADAPTER` | Query current adapter info |
| `SET_PADMAP <id>` | Override pad map selection |
| `PROVISION [<id>]` | Write adapter EEPROM (factory use) |
| `DISCOVERY_SCAN` | Sweep all 70×70 pad pairs, stream sense voltages |
| `DEBUG_BIASED_SWEEP <id>` | Debug: biased forward/reverse sweep |

### Responses (tester → host)

```
EVENT ADAPTER_DETECTED <model> <ver> [<padmap_id> ...]
EVENT ADAPTER_REMOVED
EVENT DUT_INSERTED
EVENT DUT_REMOVED
EVENT TEST_START <model> <ver> [<padmap_id> ...]
EVENT EOL_WARNING <insertion_count>

ADAPTER model=N ver=N ...
PAD <slot> <mez_pin> <GOOD|OPEN|SHORT> <prev_short> <next_short>
SUMMARY <PASS|FAIL> <good>/<tested>

DSCAN <src_mez> <snk_mez> <voltage>   (streamed during DISCOVERY_SCAN)
DSCAN DONE

ERROR <description>
```

---

## Adapter EEPROM

Each adapter carries an AT21CS01 EEPROM that stores lifetime and configuration data. The firmware reads this on adapter insertion and writes back counters after each test.

| Offset | Size | Field |
|--------|------|-------|
| 0 | 2 | Magic: `{ 0xB7, 0x72 }` |
| 2 | 1 | `adapter_model` (0x01 = Mezzanine70) |
| 3 | 1 | `adapter_version` |
| 4 | 1 | `supported_padmap_id` (0xFF = auto-detect) |
| 6 | 2 | `designed_lifespan` — rated insertion count |
| 8 | 4 | `date_of_manufacture` — Unix timestamp |
| 12 | 4 | `insertion_count` — physical DUT insertions (wear metric) |
| 16 | 4 | `test_count` — completed test runs |
| 20 | 1 | `eol_reached` (0x00 = ok, 0xFF = EOL) |
| 22 | 2 | CRC-16 over bytes 0–21 |

`insertion_count` increments on each DUT absent → present transition. When it exceeds the `designed_lifespan` for the adapter model the firmware sets `eol_reached` and drives the ID LED on the adapter board.

---

## Tools

Requires [uv](https://docs.astral.sh/uv/). Dependencies are declared inline — no separate install step.

### `tools/discovery_scan.py`

Runs a full discovery scan: sweeps every pad-pair combination and saves the resulting 74×74 voltage matrix. Useful for characterising an unknown die or verifying bond coverage before writing a formal pad map.

```bash
uv run tools/discovery_scan.py --port /dev/tty.usbmodem1101
uv run tools/discovery_scan.py --port COM3 --out my_die_scan
```

Outputs `discovery_<timestamp>.csv` and `discovery_<timestamp>.xlsx` (green→red colour scale).

| Flag | Default | Description |
|------|---------|-------------|
| `--port` | required | Serial port |
| `--baud` | 115200 | Baud rate |
| `--out` | `discovery_YYYYMMDD_HHMMSS` | Output basename (no extension) |
| `--timeout` | 120 s | Abort if scan takes longer |

### `tools/provision.py`

Writes the EEPROM on a new adapter at manufacture time. Stores model, version, pad map ID, designed lifespan, and date of manufacture.

```bash
uv run tools/provision.py --port /dev/tty.usbmodem1101 --padmap 1
uv run tools/provision.py --port COM3 --padmap 2 --date 20260101 --yes
```

| Flag | Default | Description |
|------|---------|-------------|
| `--port` | required | Serial port |
| `--baud` | 115200 | Baud rate |
| `--padmap` | required | Pad map ID (1 = 1x1 Mezzanine70 r1, 2 = 1x1 Mezzanine70 r2, 3 = 1x0p5 Mezzanine70 v1) |
| `--date` | today | Manufacture date as `YYYYMMDD` |
| `--yes` / `-y` | — | Skip confirmation prompt |

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
├── DUT_PADMAP_1X0P5.md     pad map — 1x0.5 mm die
└── RP2350 PINMAP.md      MCU GPIO assignments
tools/                    host-side Python scripts
```

See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for the full firmware design reference.

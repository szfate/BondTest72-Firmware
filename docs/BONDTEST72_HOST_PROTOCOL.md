# BondTest72 Host Protocol

USB CDC serial protocol, 115200 baud, 8N1. Lines are newline-terminated (`\n` or `\r\n`).

All communication is plain ASCII. The host sends commands (uppercase), the tester responds with events and results (uppercase). Key-value pairs use `key=value` format, space-separated.

**One CDC channel carries both the protocol and firmware logs.** Log lines (`[t] [INFO] ...`, `[t] [WARN] ...`, etc.) can be interleaved between any protocol lines — including mid-result-set, since logs are written from the same single-threaded loop. Host-side parsers must skip lines that don't match a known protocol format (the GUI tools do; they key on line prefixes like `PAD `, `EVENT `, `SUMMARY`).

---

## Protocol version

`HELLO` reports `proto=<n>`, the protocol version this document describes. It is bumped whenever a command or message format changes, so host tools can tell whether the firmware supports what they need. Firmware that sends no `proto=` speaks version 1.

| Version | Changes |
|---------|---------|
| 1 | Original protocol |
| 2 | `LIST_PADMAPS`, `SET_DEFAULT_PADMAP`; `SET_PADMAP` replies `OK` and checks adapter compatibility; `active=` on `ADAPTER`, `EVENT ADAPTER_DETECTED` and `EVENT TEST_START`; `state=` on `ADAPTER`; `type=` on `PAD` |

New fields may be added to existing lines without a version bump, so parsers must ignore keys they don't know.

---

## Pad maps

A pad map describes one die type: which adapter pins to measure, how, and which pins detect DUT presence. Pad maps are built into the firmware; `LIST_PADMAPS` returns the ones usable with the connected adapter.

- **Compatibility** is decided by the adapter's connector pinout. Any built-in pad map written for the same pinout can be selected — it does not need to appear in the adapter's EEPROM list.
- **Default:** the first entry of the adapter EEPROM's padmap list (`pm=` on `ADAPTER`) is activated whenever the adapter is detected, including at power-up. Entries this firmware doesn't know are skipped; if none are usable, the first compatible built-in map is used.
- **Active:** the pad map the next test runs with (`active=`). `SET_PADMAP` changes it until the adapter is removed or the tester restarts; `SET_DEFAULT_PADMAP` also stores it as the adapter's default.

---

## Commands

### `HELLO`

Identify the tester. Valid in any state.

**Request:** `HELLO`

**Response:**
```
HELLO name=<name> build=<build> uid=<uid> proto=<n>
```

| Field | Type | Description |
|-------|------|-------------|
| `name` | string | Device name, always `BondTest72` |
| `build` | string | Commit date and short hash, e.g. `2026-06-03-74fb8fd` |
| `uid` | string | 16-char hex unique board ID (RP2350 OTP) |
| `proto` | uint8 | Protocol version (see [Protocol version](#protocol-version)). Absent on version 1 firmware |

Recommended: send `HELLO` immediately after opening the serial port to verify the connected device and obtain firmware version.

---

### `RUN`

Start a bond test. Valid in `READY`, `PASS`, or `FAIL` state. In `PASS`/`FAIL` state, the DUT must still be present; if removed, the tester transitions to `ADAPTER_DETECTED`.

**Request:** `RUN`

**Response:** `EVENT TEST_START ...` followed by, for each slot: a `SLOT` line then that slot's `PAD` lines, and finally one `SUMMARY` line. Ignored in other states.

---

### `GET_ADAPTER`

Retrieve adapter information. Requires an adapter to be connected.

**Request:** `GET_ADAPTER`

**Response (success):**
```
ADAPTER aid=<adapter_uid> ahw=<hw> pm=<uint>[,<uint>...] active=<uint> lifespan=<n> mfg_date=<n> ins=<n> tests=<n> eol=<0|1> dut=<0|1> state=<state>
```

| Field | Type | Description |
|-------|------|-------------|
| `aid` | string | 16-char hex serial UID from the adapter's AT21CS01 EEPROM; sixteen '0's if the serial read failed. Distinct from HELLO's `uid=`, which is the tester's own RP2350 OTP board ID. |
| `ahw` | uint8 | Hardware ID (complete adapter identifier) |
| `pm` | uint8[] | Pad map IDs stored in the adapter EEPROM, comma-separated; the first is the default (absent if none) |
| `active` | uint8 | Pad map the next test will run with |
| `lifespan` | uint32 | Designed lifespan (number of insertions) |
| `mfg_date` | uint32 | Manufacturing date as YYYYMMDD |
| `ins` | uint32 | Current insertion count |
| `tests` | uint32 | Current test count |
| `eol` | 0 or 1 | End-of-life reached |
| `dut` | 0 or 1 | DUT currently present on adapter |
| `state` | string | Current tester state, one of the names in [State Machine](#state-machine) (e.g. `READY`, `PASS`) |

**Response (error):** `ERROR code=1 msg=NO_ADAPTER`, or `ERROR code=7 msg=ADAPTER_NOT_PROVISIONED` if the EEPROM is present but blank

---

### `LIST_PADMAPS`

List the built-in pad maps that can run on the connected adapter. Valid in any state.

**Request:** `LIST_PADMAPS`

**Response (success):** one `PADMAP` line per pad map, then `OK LIST_PADMAPS`.
```
PADMAP id=<uint> pads=<uint> default=<0|1> active=<0|1> name=<text>
OK LIST_PADMAPS
```

| Field | Type | Description |
|-------|------|-------------|
| `id` | uint8 | Pad map ID |
| `pads` | uint8 | Number of pads the map measures (one `PAD` line each) |
| `default` | 0 or 1 | 1 for the adapter's default pad map |
| `active` | 0 or 1 | 1 for the pad map the next test will run with |
| `name` | string | Human-readable name. **Always the last field; runs to the end of the line and may contain spaces** |

**Response (error):** `ERROR code=1 msg=NO_ADAPTER`, or `ERROR code=7 msg=ADAPTER_NOT_PROVISIONED`

---

### `SET_PADMAP`

Select the active pad map until the adapter is removed or the tester restarts. Valid in `ADAPTER_DETECTED`, `READY`, `WRONG_ORIENTATION`, `PASS` and `FAIL`.

**Request:** `SET_PADMAP id=<padmap_id>`

| Field | Type | Description |
|-------|------|-------------|
| `id` | uint8 | Pad map ID to activate |

**Response (success):** `OK SET_PADMAP id=<padmap_id>`

DUT presence and orientation are detected differently per pad map, so the tester re-checks the DUT with the new map and moves to `READY` (DUT present), `WRONG_ORIENTATION` (DUT flipped — also sends `EVENT WRONG_ORIENTATION`) or `ADAPTER_DETECTED` (no DUT). This is not counted as an insertion. A `PASS`/`FAIL` state is always left, since that result belongs to the previous pad map; `GET_RESULTS` still returns it.

**Response (error):** `ERROR code=3 msg=UNKNOWN_PADMAP` (no such ID, or `id=` missing), `ERROR code=10 msg=PADMAP_INCOMPATIBLE` (written for a different adapter pinout)

---

### `SET_DEFAULT_PADMAP`

Store a pad map as the adapter's default and activate it. Same states and checks as `SET_PADMAP`.

**Request:** `SET_DEFAULT_PADMAP id=<padmap_id>`

**Response (success):** `OK SET_DEFAULT_PADMAP id=<padmap_id>`

The ID is moved to the front of the adapter EEPROM's padmap list; the other entries keep their order (the last one is dropped if the list was already full). Counters are preserved. The EEPROM is not rewritten if the ID is already the default. After the write, the pad map is activated exactly as with `SET_PADMAP`.

**Response (error):** as `SET_PADMAP`. If the EEPROM write fails, the tester sends `EVENT FAULT msg=EEPROM_WRITE_FAILED` and enters `FAULT` instead of replying `OK`.

---

### `PROVISION`

Write configuration values to an adapter EEPROM. Requires an adapter to be physically connected. Counter values (`ins`, `tests`, `eol`) are optional — if omitted, they default to 0 (suitable for provisioning a blank adapter). To preserve existing counter values when re-provisioning, pass the current values read from `GET_ADAPTER`.

**Request:** `PROVISION hw=<id> padmap=<id>[,<id>...] lifespan=<n> date=<YYYYMMDD> [ins=<n>] [tests=<n>] [eol=<0|1>]`

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `hw` | uint8 | Yes | Hardware ID (adapter board identifier). |
| `padmap` | uint8[] | Yes | Pad map IDs to provision, comma-separated (1–4 IDs). |
| `lifespan` | uint32 | Yes | Designed lifespan — max insertions before EOL. |
| `date` | uint32 | Yes | Manufacturing date as YYYYMMDD. |
| `ins` | uint32 | No | Insertion count to write. Default: 0. |
| `tests` | uint32 | No | Test count to write. Default: 0. |
| `eol` | uint32 | No | EOL flag (0=ok, 1=end-of-life). Default: 0. |

**Response (success):** `OK PROVISION`

**Response (error):** `ERROR code=1 msg=NO_ADAPTER`, `ERROR code=6 msg=MISSING_<FIELD>`, or `ERROR code=4 msg=PROVISION_FAILED`

After successful provisioning, the tester reinitializes the adapter and transitions to `ADAPTER_DETECTED` (or `FAULT` on failure).

---

### `GET_RESULTS`

Resend the most recent test results.

**Request:** `GET_RESULTS`

**Response:** Re-sends the `SLOT`/`PAD` lines and the `SUMMARY` line from the last test, in the same order as the original `RUN` response (no `EVENT TEST_START` line is repeated).

---

## Asynchronous Events

These lines are sent by the tester without a corresponding command, in response to hardware state changes.

### `EVENT ADAPTER_DETECTED`

Sent when an adapter PCB is plugged in.

```
EVENT ADAPTER_DETECTED aid=<uid> ahw=<hw> pm=<uint>[,<uint>...] active=<uint>
```

Fields as on `ADAPTER`. `active` is the adapter's default pad map — any earlier `SET_PADMAP` choice does not carry over to a newly detected adapter.

---

### `EVENT ADAPTER_REMOVED`

Sent when the adapter is physically removed.

```
EVENT ADAPTER_REMOVED
```

---

### `EVENT DUT_INSERTED`

Sent when a DUT (die) is detected on the adapter.

```
EVENT DUT_INSERTED
```

---

### `EVENT DUT_REMOVED`

Sent when the DUT is removed.

```
EVENT DUT_REMOVED
```

---

### `EVENT TEST_START`

Sent when a test begins (after `RUN` command or button press).

```
EVENT TEST_START aid=<uid> ahw=<hw> ins=<uint> tests=<uint> pm=<uint>[,<uint>...] active=<uint> current_list_ua=<uint>[,<uint>...] [max_bond_r_ohms=<float>] [cap_time_list_us=<uint>[,<uint>...]]
```

| Field | Type | Description |
|-------|------|-------------|
| `aid` | string | 16-char adapter EEPROM serial UID |
| `ahw` | uint8 | Hardware ID |
| `ins` | uint32 | Adapter insertion count as of this test (excludes the test currently starting) |
| `tests` | uint32 | Adapter test count as of this test (excludes the test currently starting — incremented only after it completes) |
| `pm` | uint8[] | Pad map IDs stored in the adapter EEPROM, default first |
| `active` | uint8 | Pad map this test runs with — the one the following `PAD` lines belong to |
| `current_list_ua` | float[] | Nominal drive current in µA for each pullup level (10, 100, 1000): the design current through a good bond at a typical diode drop, not the short-circuit current VCC/R. Low-current-first — same order as `rf`/`rr`/`vf`/`vr` on `PAD` lines |
| `max_bond_r_ohms` | float | Resistance ceiling used to classify GOOD/OPEN (see `PAD` below). Omitted if the active pad map has no case with thresholds set |
| `cap_time_list_us` | uint16[] | Elapsed-time schedule (µs) for the 5 CAP_SENSE curve samples — same order as `vfs`/`vrs` on CAP `PAD` lines. Omitted if the active pad map has no CAP_SENSE case |

---

### `EVENT EOL_WARNING`

Sent when the adapter has reached its end-of-life insertion count.

```
EVENT EOL_WARNING ins=<count>
```

---

### `EVENT WRONG_ORIENTATION`

Sent when the DUT is inserted backwards (detected via continuity check).

```
EVENT WRONG_ORIENTATION
```

---

### `EVENT FAULT`

Sent on internal faults.

```
EVENT FAULT msg=<message>
```

---

## Test Result Lines

### `PAD`

One line per tested die pad, sent during a test run. Format depends on `method`:
- `STD` / `STD_FW` / `STD_REV` — most IO pads. A Kelvin resistance sweep: 3 pullup levels (280k/27.4k/2.49k). The variant encodes which direction group(s) the line carries: `STD` = both, `STD_FW` = forward only, `STD_REV` = reverse only.
- `CAP` / `CAP_FW` / `CAP_REV` — pads with a real bypass/decoupling cap (VDDIO/VDD_CORE/PWR_AUX), which can't settle fast enough at 280k/27.4k. Instead of resistance, reports raw voltage samples across the 2.49k-only charging curve. Variants as for `STD`.

The measured direction(s) are set per pad map (`PadMap::directions` in `src/test/pad_map.h`). Most pad maps measure the reverse direction only: forward drive charges the adapter-side bypass cap to ~VCC and is the trigger sequence for the CH446X latch-up on r1 boards. Their PAD lines carry `method=STD_REV`/`CAP_REV` and only the reverse groups; capless DUTs (MOSB) measure both. Unmeasured groups are never sent as zeros — `0Ω` would read as a dead short.

```
PAD slot=<slot> apin=<adapter_pin> dp=<die_pad> type=<type> method=STD_REV result=<GOOD|OPEN> rr=<f,f,f> vr=<f,f,f>
PAD slot=<slot> apin=<adapter_pin> dp=<die_pad> type=<type> method=CAP_REV result=<GOOD|OPEN> vrs=<f,f,f,f,f>
```

| Field | Type | Description |
|-------|------|-------------|
| `slot` | uint8 | DUT slot number (0 for single-DUT adapters) |
| `apin` | uint8 | Adapter pin number |
| `dp` | uint8 | Die pad number (0-indexed) |
| `type` | string | Pad role from the pad map: `IO`, `VDDIO`, `VDD_CORE`, `PWR_AUX` or `GND` |
| `method` | string | `STD`/`STD_FW`/`STD_REV` or `CAP`/`CAP_FW`/`CAP_REV` — which direction group(s) the line carries |
| `result` | string | `GOOD` or `OPEN` |
| `rr` | float[3] | STD lines. Reverse apparent bond resistance (Ω) at each pullup level, low-current-first (280k, 27.4k, 2.49k — see `current_list_ua` on `EVENT TEST_START`). GOOD if any of the reverse readings is below `max_bond_r_ohms`. |
| `vr` | float[3] | STD lines. The underlying Kelvin voltages behind `rr`, same order, 3 decimal places. |
| `rf` / `vf` | float[3] | STD lines. Forward-direction equivalents of `rr`/`vr`, present only when the method includes forward (`STD`, `STD_FW`). |
| `vrs` | float[5] | CAP lines. Raw voltage samples across the reverse charging curve, earliest-first (see `cap_time_list_us` on `EVENT TEST_START`), 3 decimal places. No resistance is reported — mid-charge resistance isn't physically meaningful; classification uses only the final (most-settled) sample. Present only when the method includes reverse (`CAP`, `CAP_REV`). |
| `vfs` | float[5] | CAP lines. Forward charging curve, present only when the method includes forward (`CAP`, `CAP_FW`). |

---

### `SLOT`

Sent before that slot's `PAD` lines (one `SLOT` line per slot, immediately followed by its pads). If `tested=0`, no `PAD` lines follow for that slot.

```
SLOT slot=<slot> present=<0|1> tested=<0|1>
```

---

### `SUMMARY`

Final line of a test run.

```
SUMMARY outcome=<result> good=<n> tested=<n> [fail_reason=DUT_REMOVED]
```

| Field | Type | Description |
|-------|------|-------------|
| `outcome` | string | `PASS` or `FAIL` |
| `good` | uint8 | Number of pads that passed |
| `tested` | uint8 | Total pads tested |
| `fail_reason` | string | Optional. Only present if outcome is `FAIL` and the DUT was removed mid-test |

---

## Error Responses

```
ERROR code=<code> msg=<message>
```

| Code | Name | Description |
|------|------|-------------|
| 1 | `NO_ADAPTER` | No adapter PCB is connected |
| 2 | `BUSY` | Tester is currently running a test |
| 3 | `UNKNOWN_PADMAP` | The requested pad map ID is not available |
| 4 | `PROVISION_FAILED` | EEPROM provisioning failed |
| 5 | `NOT_IMPLEMENTED` | Feature not yet implemented |
| 6 | `MISSING_FIELD` | Required PROVISION field omitted (msg=MISSING_HW, MISSING_PADMAP, MISSING_LIFESPAN, or MISSING_DATE) |
| 7 | `ADAPTER_NOT_PROVISIONED` | Adapter EEPROM chip present but blank — needs PROVISION before use |
| 8 | `WRONG_STATE` | Command not valid in the tester's current state (msg = current state name; see the per-state table below) |
| 9 | `EEPROM_WRITE_FAILED` | **Reserved** — never emitted. A runtime flush that fails even after write-verify + one retry now escalates to `EVENT FAULT msg=EEPROM_WRITE_FAILED` and the FAULT state instead |
| 10 | `PADMAP_INCOMPATIBLE` | The pad map exists but is written for a different adapter pinout |

---

## State Machine

The tester operates as a state machine. Some commands are only valid in certain states.

```
  NO_ADAPTER ──adapter inserted──▸ ADAPTER_DETECTED
  ADAPTER_DETECTED ──DUT inserted──▸ READY
  ADAPTER_DETECTED ──EOL reached──▸ EOL_ADAPTER
  READY ──RUN──▸ TESTING ──pass──▸ PASS
  READY ──RUN──▸ TESTING ──fail──▸ FAIL
  READY ──wrong orientation──▸ WRONG_ORIENTATION
  PASS/FAIL ──DUT removed──▸ ADAPTER_DETECTED
  ADAPTER_DETECTED/READY/WRONG_ORIENTATION/PASS/FAIL ──SET_PADMAP──▸ READY / WRONG_ORIENTATION / ADAPTER_DETECTED (re-sensed DUT)
  any ──fault──▸ FAULT
```

| State | Valid commands |
|-------|---------------|
Every state also accepts the queries `HELLO`, `GET_ADAPTER` and `LIST_PADMAPS`.

| State | Valid commands (besides the queries) |
|-------|---------------|
| `NO_ADAPTER` | PROVISION |
| `FAULT` | PROVISION |
| `ADAPTER_DETECTED` | SET_PADMAP, SET_DEFAULT_PADMAP, GET_RESULTS, PROVISION |
| `READY` | RUN, SET_PADMAP, SET_DEFAULT_PADMAP, GET_RESULTS, PROVISION |
| `TESTING` | — |
| `PASS` / `FAIL` | RUN, SET_PADMAP, SET_DEFAULT_PADMAP, GET_RESULTS |
| `EOL_ADAPTER` | PROVISION |
| `WRONG_ORIENTATION` | SET_PADMAP, SET_DEFAULT_PADMAP |

The queries are valid in every state.

**Commands sent during a test are queued, not rejected.** A test runs to completion without reading the serial port, so anything the host sends meanwhile is handled right after the result lines — in `PASS`/`FAIL`, not `TESTING`. A `SET_PADMAP` queued this way is therefore accepted and immediately leaves the `PASS`/`FAIL` state. Hosts should wait for `SUMMARY` before changing the pad map. `PROVISION`
is allowed everywhere except `TESTING`: a freshly manufactured (blank) adapter
latches the tester into `FAULT`, and provisioning from there is the normal
entry path — the old "EOL_ADAPTER only" rule was wrong. Commands outside a
state's set are answered with `ERROR 8 WRONG_STATE` naming the current state.

---

## Typical Host Session

```
# Host opens serial port and verifies device
→ HELLO
← HELLO name=BondTest72 build=2026-06-03-74fb8fd uid=A1B2C3D4E5F60718 proto=2

# Adapter plugged in (asynchronous)
← EVENT ADAPTER_DETECTED aid=0123456789ABCDEF ahw=1 pm=2 active=2

# Host lists the pad maps and picks one for this session
→ LIST_PADMAPS
← PADMAP id=2 pads=64 default=1 active=1 name=1x1 v2
← PADMAP id=3 pads=64 default=0 active=0 name=1x0p5 v1
← OK LIST_PADMAPS
→ SET_PADMAP id=3
← OK SET_PADMAP id=3

# DUT inserted (asynchronous)
← EVENT DUT_INSERTED

# Host starts test
→ RUN
← EVENT TEST_START aid=0123456789ABCDEF ahw=1 ins=237 tests=263 pm=2 active=3 current_list_ua=10,100,1000 max_bond_r_ohms=60000 cap_time_list_us=156,312,1250,10000,20000
← SLOT slot=0 present=1 tested=1
← PAD slot=0 apin=63 dp=0 type=IO method=STD_REV result=GOOD rr=53579,6734,1097 vr=0.461,0.559,0.824
← PAD slot=0 apin=64 dp=1 type=IO method=STD_REV result=GOOD rr=56431,6957,1105 vr=0.482,0.575,0.828
  ... (one PAD line per die pad; CAP pads use method=CAP_REV and vrs= instead of rr=/vr=)
← SUMMARY outcome=PASS good=64 tested=64

# Host requests results again
→ GET_RESULTS
← SLOT slot=0 present=1 tested=1
← PAD slot=0 apin=63 dp=0 type=IO method=STD_REV result=GOOD rr=53579,6734,1097 vr=0.461,0.559,0.824
  ... (all SLOT/PAD/SUMMARY lines repeated; no EVENT TEST_START)
```

---

## Protocol Versioning

There is no explicit version negotiation field. The `HELLO` response includes a `build` identifier (git commit date and hash) that the host can use to determine protocol compatibility.
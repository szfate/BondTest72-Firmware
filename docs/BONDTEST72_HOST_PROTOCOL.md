# BondTest72 Host Protocol

USB CDC serial protocol, 115200 baud, 8N1. Lines are newline-terminated (`\n` or `\r\n`).

All communication is plain ASCII. The host sends commands (uppercase), the tester responds with events and results (uppercase). Key-value pairs use `key=value` format, space-separated.

---

## Commands

### `HELLO`

Identify the tester. Valid in any state.

**Request:** `HELLO`

**Response:**
```
HELLO name=<name> build=<build> uid=<uid>
```

| Field | Type | Description |
|-------|------|-------------|
| `name` | string | Device name, always `BondTest72` |
| `build` | string | Commit date and short hash, e.g. `2026-06-03-74fb8fd` |
| `uid` | string | 16-char hex unique board ID (RP2350 OTP) |

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
ADAPTER aid=<uid> ahw=<hw> pm=<uint>[,<uint>...] lifespan=<n> mfg_date=<n> ins=<n> tests=<n> eol=<0|1> dut=<0|1>
```

| Field | Type | Description |
|-------|------|-------------|
| `aid` | string | 16-char adapter EEPROM serial UID |
| `ahw` | uint8 | Hardware ID (complete adapter identifier) |
| `pm` | uint8[] | Supported pad map IDs, comma-separated (absent if none) |
| `lifespan` | uint32 | Designed lifespan (number of insertions) |
| `mfg_date` | uint32 | Manufacturing date as YYYYMMDD |
| `ins` | uint32 | Current insertion count |
| `tests` | uint32 | Current test count |
| `eol` | 0 or 1 | End-of-life reached |
| `dut` | 0 or 1 | DUT currently present on adapter |

**Response (error):** `ERROR code=1 msg=NO_ADAPTER`, or `ERROR code=7 msg=ADAPTER_NOT_PROVISIONED` if the EEPROM is present but blank

---

### `SET_PADMAP`

Select the active pad map. Valid in any state with an adapter present.

**Request:** `SET_PADMAP id=<padmap_id>`

| Field | Type | Description |
|-------|------|-------------|
| `id` | uint8 | Pad map ID to activate |

**Response (success):** Pad map is set internally. No confirmation line is sent.

**Response (error):** `ERROR code=3 msg=UNKNOWN_PADMAP`

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

### `DISCOVERY_SCAN`

Run a full pad-to-pad voltage sweep on all 70×70 combinations. Requires adapter and pad map. Not valid during test (`TESTING` state).

**Request:** `DISCOVERY_SCAN`

**Response:** Emits 4900 `DSCAN` lines followed by `DSCAN DONE`. Each line:
```
DSCAN src=<src> snk=<snk> sv=<voltage>
```

**Response (error):** `ERROR code=2 msg=BUSY` or `ERROR code=1 msg=NO_ADAPTER`

---

## Asynchronous Events

These lines are sent by the tester without a corresponding command, in response to hardware state changes.

### `EVENT ADAPTER_DETECTED`

Sent when an adapter PCB is plugged in.

```
EVENT ADAPTER_DETECTED aid=<uid> ahw=<hw> pm=<uint>[,<uint>...]
```

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
EVENT TEST_START aid=<uid> ahw=<hw> ins=<uint> tests=<uint> pm=<uint>[,<uint>...] current_list_ua=<uint>[,<uint>...] [max_bond_r_ohms=<float>] [cap_time_list_us=<uint>[,<uint>...]]
```

| Field | Type | Description |
|-------|------|-------------|
| `aid` | string | 16-char adapter EEPROM serial UID |
| `ahw` | uint8 | Hardware ID |
| `ins` | uint32 | Adapter insertion count as of this test (excludes the test currently starting) |
| `tests` | uint32 | Adapter test count as of this test (excludes the test currently starting — incremented only after it completes) |
| `pm` | uint8[] | Supported pad map IDs, comma-separated |
| `current_list_ua` | float[] | Drive current in µA for each pullup level, low-current-first — same order as `rf`/`rr`/`vf`/`vr` on `PAD` lines |
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
- `STD` / `STD_FW` / `STD_REV` — most IO pads. A Kelvin resistance sweep: 3 pullup levels (330k/33k/3.3k). The variant encodes which direction group(s) the line carries: `STD` = both, `STD_FW` = forward only, `STD_REV` = reverse only.
- `CAP` / `CAP_FW` / `CAP_REV` — pads with a real bypass/decoupling cap (VDDIO/VDD_CORE/PWR_AUX), which can't settle fast enough at 330k/33k. Instead of resistance, reports raw voltage samples across the 3.3k-only charging curve. Variants as for `STD`.

The current firmware measures the reverse direction only (`MEASURE_DIRECTIONS = REVERSE_ONLY` in `src/test/result.h`): forward drive charges the adapter-side bypass cap to ~VCC and is the trigger sequence for the CH446X latch-up. PAD lines therefore carry `method=STD_REV`/`CAP_REV` and only the reverse groups. Unmeasured groups are never sent as zeros — `0Ω` would read as a dead short.

```
PAD slot=<slot> apin=<adapter_pin> dp=<die_pad> method=STD_REV result=<GOOD|OPEN> rr=<f,f,f> vr=<f,f,f>
PAD slot=<slot> apin=<adapter_pin> dp=<die_pad> method=CAP_REV result=<GOOD|OPEN> vrs=<f,f,f,f,f>
```

| Field | Type | Description |
|-------|------|-------------|
| `slot` | uint8 | DUT slot number (0 for single-DUT adapters) |
| `apin` | uint8 | Adapter pin number |
| `dp` | uint8 | Die pad number (0-indexed) |
| `method` | string | `STD`/`STD_FW`/`STD_REV` or `CAP`/`CAP_FW`/`CAP_REV` — which direction group(s) the line carries |
| `result` | string | `GOOD` or `OPEN` |
| `rr` | float[3] | STD lines. Reverse apparent bond resistance (Ω) at each pullup level, low-current-first (330k, 33k, 3.3k — see `current_list_ua` on `EVENT TEST_START`). GOOD if any of the reverse readings is below `max_bond_r_ohms`. |
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
  any ──fault──▸ FAULT
```

| State | Valid commands |
|-------|---------------|
| `NO_ADAPTER` | HELLO |
| `ADAPTER_DETECTED` | HELLO, SET_PADMAP, GET_ADAPTER, DISCOVERY_SCAN |
| `READY` | HELLO, RUN, SET_PADMAP, GET_ADAPTER, DISCOVERY_SCAN |
| `TESTING` | HELLO |
| `PASS` / `FAIL` | HELLO, RUN, GET_RESULTS |
| `EOL_ADAPTER` | HELLO, PROVISION |
| `WRONG_ORIENTATION` | HELLO |
| `FAULT` | HELLO |

---

## Discovery Scan Output

The `DSCAN` output format is separate from normal test results:

```
DSCAN src=<src> snk=<snk> sv=<voltage>
```

- `src`: source die pad (0-indexed)
- `snk`: sink die pad (0-indexed)
- `sv`: measured sense voltage, 3 decimal places

The scan ends with:
```
DSCAN DONE
```

---

## Typical Host Session

```
# Host opens serial port and verifies device
→ HELLO
← HELLO name=BondTest72 build=2026-06-03-74fb8fd uid=A1B2C3D4E5F60718

# Adapter plugged in (asynchronous)
← EVENT ADAPTER_DETECTED aid=0123456789ABCDEF ahw=1 pm=2

# DUT inserted (asynchronous)
← EVENT DUT_INSERTED

# Host starts test
→ RUN
← EVENT TEST_START aid=0123456789ABCDEF ahw=1 ins=237 tests=263 pm=2 current_list_ua=10,100,1000 max_bond_r_ohms=60000 cap_time_list_us=1250,2500,10000,15000,20000
← SLOT slot=0 present=1 tested=1
← PAD slot=0 apin=63 dp=0 method=STD_REV result=GOOD rr=53579,6734,1097 vr=0.461,0.559,0.824
← PAD slot=0 apin=64 dp=1 method=STD_REV result=GOOD rr=56431,6957,1105 vr=0.482,0.575,0.828
  ... (one PAD line per die pad; CAP pads use method=CAP_REV and vrs= instead of rr=/vr=)
← SUMMARY outcome=PASS good=64 tested=64

# Host requests results again
→ GET_RESULTS
← SLOT slot=0 present=1 tested=1
← PAD slot=0 apin=63 dp=0 method=STD_REV result=GOOD rr=53579,6734,1097 vr=0.461,0.559,0.824
  ... (all SLOT/PAD/SUMMARY lines repeated; no EVENT TEST_START)
```

---

## Protocol Versioning

There is no explicit version negotiation field. The `HELLO` response includes a `build` identifier (git commit date and hash) that the host can use to determine protocol compatibility.
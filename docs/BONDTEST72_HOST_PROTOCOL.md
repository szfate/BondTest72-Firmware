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

**Response:** `EVENT TEST_START ...` followed by `PAD` lines and a `SUMMARY` line. Ignored in other states.

---

### `GET_ADAPTER`

Retrieve adapter information. Requires an adapter to be connected.

**Request:** `GET_ADAPTER`

**Response (success):**
```
ADAPTER uid=<uid> hw=<hw> pm=<uint>[,<uint>...] lifespan=<n> mfg_date=<n> ins=<n> tests=<n> eol=<0|1> dut=<0|1>
```

| Field | Type | Description |
|-------|------|-------------|
| `uid` | string | 16-char adapter EEPROM serial UID |
| `hw` | uint8 | Hardware ID (complete adapter identifier) |
| `pm` | uint8[] | Supported pad map IDs, comma-separated (absent if none) |
| `lifespan` | uint32 | Designed lifespan (number of insertions) |
| `mfg_date` | uint32 | Manufacturing date as YYYYMMDD |
| `ins` | uint32 | Current insertion count |
| `tests` | uint32 | Current test count |
| `eol` | 0 or 1 | End-of-life reached |
| `dut` | 0 or 1 | DUT currently present on adapter |

**Response (error):** `ERROR code=1 msg=NO_ADAPTER`

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

**Response:** Re-sends all `PAD` lines, `SLOT` lines, and the `SUMMARY` line from the last test. If no test has been run, no response is sent.

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
EVENT ADAPTER_DETECTED uid=<uid> hw=<hw> pm=<uint>[,<uint>...]
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
EVENT TEST_START uid=<uid> hw=<hw> pm=<uint>[,<uint>...]
```

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

One line per tested die pad, sent during a test run.

```
PAD slot=<slot> apin=<adapter_pin> dp=<die_pad> result=<result> ps=<prev_short> ns=<next_short> sv=<sense_V> pv=<prev_V> nv=<next_V>
```

| Field | Type | Description |
|-------|------|-------------|
| `slot` | uint8 | DUT slot number (0 for single-DUT adapters) |
| `apin` | uint8 | Adapter pin number |
| `dp` | uint8 | Die pad number (0-indexed) |
| `result` | string | `GOOD`, `OPEN`, or `SHORT` |
| `ps` | 0 or 1 | Previous-neighbour short detected |
| `ns` | 0 or 1 | Next-neighbour short detected |
| `sv` | float | Sense voltage (COM_D), 3 decimal places |
| `pv` | float | Previous-neighbour voltage (COM_A), 3 decimal places |
| `nv` | float | Next-neighbour voltage (COM_C), 3 decimal places |

---

### `SLOT`

Sent after all pads in a slot have been tested.

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
← EVENT ADAPTER_DETECTED uid=0123456789ABCDEF hw=1 pm=2

# DUT inserted (asynchronous)
← EVENT DUT_INSERTED

# Host starts test
→ RUN
← EVENT TEST_START uid=0123456789ABCDEF hw=1 pm=2
← PAD slot=0 apin=1 dp=0 result=GOOD ps=0 ns=0 sv=0.650 pv=1.600 nv=1.580
← PAD slot=0 apin=2 dp=1 result=GOOD ps=0 ns=0 sv=0.620 pv=1.610 nv=1.590
  ... (one PAD line per die pad)
← SLOT slot=0 present=1 tested=1
← SUMMARY outcome=PASS good=62 tested=64

# Host requests results again
→ GET_RESULTS
← PAD slot=0 apin=1 dp=0 result=GOOD ps=0 ns=0 sv=0.650 pv=1.600 nv=1.580
  ... (all PAD/SLOT/SUMMARY lines repeated)
```

---

## Protocol Versioning

There is no explicit version negotiation field. The `HELLO` response includes a `build` identifier (git commit date and hash) that the host can use to determine protocol compatibility.
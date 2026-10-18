# BondTest72 Results File Format (JSONL)

Test results are stored as [JSON Lines](https://jsonlines.org/): one JSON object per line, one line per completed test of one DUT slot. Files are append-only; the GUI writes one file per day, named `YYYY-MM-DD.jsonl`.

The format is shared by the BondTest72 GUI (`gui/src/bondtest72_gui/results_jsonl.py`) and the factory tool, so files from either can be opened by both. Field values come from the tester's serial protocol (`docs/BONDTEST72_HOST_PROTOCOL.md`); where a field mirrors a protocol field, that document has the full meaning.

Readers should ignore keys they don't know, and skip lines they can't parse — a crash mid-write can leave a damaged last line.

## Example

One record, pretty-printed (on disk it is a single line):

```json
{
  "ts": "2026-10-15T14:03:27.512",
  "outcome": "PASS",
  "good": 64,
  "tested": 64,
  "reason": null,
  "slot": 0,
  "pads": [
    {"apin": 63, "dp": 0, "method": "STD_REV", "result": "GOOD", "role": "IO",
     "rr": [48351.0, 5944.0, 894.0], "vr": [0.486, 0.588, 0.872]},
    {"apin": 9, "dp": 17, "method": "CAP_REV", "result": "GOOD", "role": "VDDIO",
     "vrs": [0.334, 0.392, 0.619, 0.604, 0.607]}
  ],
  "img": null,
  "panel_id": "sample-07",
  "context": {
    "aid": "0123456789ABCDEF", "ahw": 2, "ins": 12, "tests": 30, "pm": 2,
    "currentListUa": [10.0, 100.0, 1000.0],
    "testerUid": "A1B2C3D4E5F60718", "testerBuild": "2026-10-15-0415793",
    "maxBondROhms": 60000.0,
    "capTimeListUs": [156, 312, 1250, 10000, 20000],
    "pmName": "1x1 v2"
  }
}
```

## Record fields

| Key | Type | Description |
|-----|------|-------------|
| `ts` | string | Local time the result was recorded, ISO 8601 with milliseconds, no timezone |
| `outcome` | string | `PASS` or `FAIL` (`SUMMARY outcome=`) |
| `good` | int | Pads that passed |
| `tested` | int | Pads measured |
| `reason` | string or null | Extra failure cause, e.g. `DUT_REMOVED` (`SUMMARY fail_reason=`) |
| `slot` | int | DUT slot (0 on single-DUT adapters) |
| `pads` | array | One object per measured pad, in test order — see below |
| `img` | string or null | Photo of the DUT, path relative to the results folder. Factory tool only |
| `panel_id` | string or null | Sample label entered by the user (factory tool: panel ID) |
| `thumb` | string | Optional. Thumbnail of `img`. Factory tool only |
| `context` | object | Optional. Tester and test settings — see below |

## Pad fields

| Key | Type | Description |
|-----|------|-------------|
| `apin` | int | Adapter pin |
| `dp` | int | Die pad number |
| `method` | string | `STD`/`STD_FW`/`STD_REV` (resistance sweep) or `CAP`/`CAP_FW`/`CAP_REV` (charging curve); the suffix tells which direction(s) were measured |
| `result` | string | `GOOD` or `OPEN` |
| `role` | string | `IO`, `VDDIO`, `VDD_CORE`, `PWR_AUX` or `GND`. Records from tester firmware before protocol 2 may say `IO` for supply pads |
| `rr`, `rf` | float[3] | STD pads: apparent bond resistance in Ω (reverse, forward), one per drive current in `currentListUa` order |
| `vr`, `vf` | float[3] | STD pads: the Kelvin voltages behind `rr`/`rf` |
| `vrs`, `vfs` | float[5] | CAP pads: charging-curve voltages sampled at `capTimeListUs` |

Measurement keys are present only for the directions that were measured; they are never zero-filled.

## Context fields

| Key | Type | Description |
|-----|------|-------------|
| `aid` | string | Adapter serial number |
| `ahw` | int | Adapter hardware ID |
| `ins` | int | Adapter insertion count when the test started |
| `tests` | int | Adapter test count when the test started |
| `pm` | int | Pad map the test ran with |
| `pmName` | string | Optional. That pad map's name |
| `currentListUa` | float[] | Nominal drive current in µA per STD reading, lowest first (10, 100, 1000) |
| `maxBondROhms` | float | Optional. Resistance below which a reading counts as conducting |
| `capTimeListUs` | int[] | Optional. Sample times in µs of the CAP charging curve |
| `testerUid` | string | Tester board ID |
| `testerBuild` | string | Tester firmware build |

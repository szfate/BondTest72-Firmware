# BondTest72 GUI

Desktop app for the BondTest72 wirebond tester: choose the pad map, run tests,
and view and record per-pad results.

Work in progress. The package currently contains the app's core, without a user
interface:

| Module | Purpose |
|--------|---------|
| `protocol.py` | Parse tester lines and build commands (`docs/BONDTEST72_HOST_PROTOCOL.md`) |
| `assembler.py` | Collect a test's protocol lines into one result |
| `results_jsonl.py` | Read and write results files (JSON Lines, one test per line) |
| `die_shapes.py` | Die outlines and pad rings for drawing results |
| `link.py` | Find the tester's serial port and read lines in the background |

## Development

```sh
cd gui
uv run pytest
```

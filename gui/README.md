# BondTest72 GUI

Desktop app for the BondTest72 wirebond tester: choose the pad map, run tests,
and view and record per-pad results.

Work in progress. Run it with:

```sh
cd gui
uv run gui
```

It connects to the tester automatically when exactly one is plugged in. Live
results are appended to a daily JSONL file in the results folder
(File → Results folder; default `results/` under the folder the app was
started from), tagged with the
optional sample ID. File → Open results file views earlier results, including
the factory tool's.

The core is kept free of Qt so it can be used from scripts and tests:

| Module | Purpose |
|--------|---------|
| `protocol.py` | Parse tester lines and build commands (`docs/BONDTEST72_HOST_PROTOCOL.md`) |
| `assembler.py` | Collect a test's protocol lines into one result |
| `session.py` | Track tester state (adapter, pad maps, state) and the queries that keep it fresh |
| `results_jsonl.py` | Read and write results files (JSON Lines, one test per line) |
| `die_shapes.py` | Die outlines and pad rings for drawing results |
| `tester_serial.py` | Find the tester's serial port and read lines in the background |
| `formatting.py` | Text for measurements (Ω, V) |

The Qt user interface:

| Module | Purpose |
|--------|---------|
| `app.py` | Entry point |
| `main_window.py` | Main window: connection, pad map, run, results history, serial log |
| `connection.py` | Moves serial lines onto the Qt event loop and feeds the session |
| `die_view.py` | Die drawing with pads coloured by result |
| `results_panel.py` | One pad group's results: sortable table with its graph underneath |
| `pad_table.py` | Per-pad table, one column per voltage reading |
| `charts.py` | IO pad voltages and power pad charging curves (pyqtgraph) |

## Development

```sh
cd gui
uv run pytest
```

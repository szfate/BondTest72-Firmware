# BondTest72 Tools

All scripts use [uv](https://github.com/astral-sh/uv) for dependency management and can be run directly without a manual `pip install` step.

## Prerequisites

Install `uv` if you don't have it:

```sh
curl -LsSf https://astral.sh/uv/install.sh | sh
```

---

## Viewing results

The die map viewers that used to live here (`live_serial_viewer.py`, `die_visualizer`) have been replaced by
the desktop app in [`gui/`](../gui/README.md), which shows live tests and opens saved results files.

---

## provision.py

Writes the adapter EEPROM: sets hardware ID, pad map ID, and manufacture date. Queries the current adapter state first and asks for confirmation before writing. On re-provisioning, the insertion/test counters and manufacture date are kept; if the current record can't be read (and the adapter isn't blank), it refuses rather than resetting the counters to 0.

```sh
uv run tools/provision.py --port /dev/tty.usbmodem1101 --hw 1 --padmap 2
uv run tools/provision.py --port COM3 --hw 1 --padmap 3 --date 20260601 --yes
```

| Flag | Default | Description |
|------|---------|-------------|
| `--port` | *(required)* | Serial port |
| `--baud` | `115200` | Baud rate |
| `--hw` | *(required)* | Hardware ID (1 = Mezzanine70) |
| `--padmap` | *(required)* | Pad map IDs, comma-separated (1 = Mezzanine70 COB v1, 2 = Mezzanine70 COB v2, 3 = Mezzanine70 COB v1 1×0.5, 4 = G801-MOSB, 5 = Mezzanine70 0.5×1, 6 = G801-TQVA) |
| `--lifespan` | existing, else `250` | Max insertions before EOL. Raising it above the insertion count clears the EOL flag; lowering it to the insertion count or below is refused (counters kept) |
| `--date` | existing, else today | Manufacture date in `YYYYMMDD` format |
| `--override` | `false` | Reset insertion/test counters and EOL to 0; also allows provisioning an adapter whose record can't be read, or a lifespan at or below the old insertion count |
| `--yes` / `-y` | `false` | Skip confirmation prompt |

---

## setup_boards.py

Production setup loop. Builds the firmware once, then for each board you connect: flashes it, reads the tester UID (`HELLO`), provisions the adapter EEPROM, reads back the adapter UID (`GET_ADAPTER`) and appends a row to a CSV database.

```sh
uv run tools/setup_boards.py --hw 2 --padmap 2
uv run tools/setup_boards.py --hw 2 --padmap 2,3 --db ~/bondtest72_boards.csv
uv run tools/setup_boards.py --hw 2 --padmap 2 --no-flash      # provision + log only
uv run tools/setup_boards.py --no-flash --no-provision         # just log UIDs
```

Flashing copies the UF2 to the `RP2350` BOOTSEL drive (more reliable here than `picotool`). A blank board shows up as that drive directly; a board already running firmware is rebooted into BOOTSEL with a 1200-baud touch. After flashing, the tester's reported build must match the one just built.

If an adapter is already provisioned, its insertion/test counters and manufacture date are kept. A UID that is already in the database is logged again, with a note on screen.

Database columns: `timestamp, tester_uid, fw_build, proto, adapter_uid, adapter_hw, padmaps, lifespan, mfg_date, ins, tests, flashed, provisioned, note`.

| Flag | Default | Description |
|------|---------|-------------|
| `--hw` | *(required unless `--no-provision`)* | Adapter hardware ID (1 = Mezzanine70 r1, 2 = Mezzanine70 r2) |
| `--padmap` | *(required unless `--no-provision`)* | Pad map IDs, comma-separated |
| `--lifespan` | existing, else `9999` | Max insertions before EOL. Raising it above the insertion count clears the EOL flag; lowering it to the insertion count or below is refused (counters kept) |
| `--date` | existing, else today | Manufacture date `YYYYMMDD` |
| `--db` | `tools/boards.csv` | CSV database file |
| `--uf2` | build | Flash this UF2 instead of building |
| `--no-flash` | `false` | Skip building and flashing |
| `--no-provision` | `false` | Only log UIDs |
| `--port` | auto | Serial port (auto-detected by Raspberry Pi USB VID) |
| `--bootsel` | auto | BOOTSEL drive mount point (`/Volumes/RP2350`, `/media/$USER/RP2350`) |
| `--note` | — | Note added to every row |
| `--once` | `false` | Set up one board and exit |
| `--yes` / `-y` | `false` | Unattended: no Enter/note prompts; waits for unplug → next board, stops on first failure |

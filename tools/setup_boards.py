#!/usr/bin/env -S uv run
# /// script
# requires-python = ">=3.11"
# dependencies = [
#   "pyserial",
# ]
# ///
"""
setup_boards.py — production setup loop for BondTest72 testers + adapters

For each board connected in turn:
  1. flash the firmware (UF2 copy to the RP2350 BOOTSEL drive)
  2. read the tester UID and firmware build (HELLO)
  3. provision the adapter EEPROM (PROVISION), preserving counters if already provisioned
  4. read back the adapter UID (GET_ADAPTER)
  5. append a row to a CSV database

The firmware is built once at start-up (pio run) unless --uf2 or --no-flash is given.
A board that is already running firmware is rebooted into BOOTSEL with a 1200-baud
touch; a blank board shows up directly as the BOOTSEL drive.

Usage:
    uv run tools/setup_boards.py --hw 2 --padmap 2
    uv run tools/setup_boards.py --hw 2 --padmap 2,3 --db ~/bondtest72_boards.csv
    uv run tools/setup_boards.py --hw 2 --padmap 2 --no-flash          # provision + log only
    uv run tools/setup_boards.py --no-flash --no-provision             # just log UIDs
"""

import argparse
import csv
import os
import re
import shutil
import subprocess
import sys
import time
from datetime import date, datetime
from pathlib import Path

try:
    import serial
    import serial.tools.list_ports
except ImportError:
    print("ERROR: pyserial not installed.")
    sys.exit(1)

sys.path.insert(0, str(Path(__file__).resolve().parent))
from provision import (  # noqa: E402
    ADAPTER_HW_NAMES,
    PAD_MAPS,
    parse_error_line,
    print_adapter_info,
    query_adapter,
    send_provision,
)

REPO_ROOT = Path(__file__).resolve().parent.parent
DEFAULT_UF2 = REPO_ROOT / ".pio" / "build" / "pico2" / "firmware.uf2"
BUILD_INFO_H = REPO_ROOT / "src" / "build_info.h"
DEFAULT_DB = Path(__file__).resolve().parent / "boards.csv"

RPI_VID = 0x2E8A
BOOTSEL_VOLUME_NAME = "RP2350"

DB_FIELDS = [
    "timestamp", "tester_uid", "fw_build", "proto",
    "adapter_uid", "adapter_hw", "padmaps", "lifespan", "mfg_date",
    "ins", "tests", "flashed", "provisioned", "note",
]


# ── Build ─────────────────────────────────────────────────────────────────────

def find_pio() -> str | None:
    pio = shutil.which("pio")
    if pio:
        return pio
    fallback = Path.home() / ".platformio" / "penv" / "bin" / "pio"
    return str(fallback) if fallback.exists() else None


def build_firmware() -> bool:
    pio = find_pio()
    if not pio:
        print("ERROR: pio not found on PATH or in ~/.platformio/penv/bin.")
        return False
    print(f"Building firmware ({pio} run) …")
    result = subprocess.run([pio, "run", "-e", "pico2"], cwd=REPO_ROOT)
    return result.returncode == 0


def read_build_id() -> str | None:
    try:
        m = re.search(r'#define FW_BUILD_ID\s+"([^"]+)"', BUILD_INFO_H.read_text())
    except OSError:
        return None
    return m.group(1) if m else None


# ── Device discovery ──────────────────────────────────────────────────────────

def bootsel_candidates() -> list[Path]:
    user = os.environ.get("USER", "")
    return [
        Path("/Volumes") / BOOTSEL_VOLUME_NAME,
        Path("/media") / user / BOOTSEL_VOLUME_NAME,
        Path("/run/media") / user / BOOTSEL_VOLUME_NAME,
    ]


def find_bootsel_drive(explicit: Path | None) -> Path | None:
    for p in [explicit] if explicit else bootsel_candidates():
        if (p / "INFO_UF2.TXT").exists():
            return p
    return None


def find_tester_ports() -> list[str]:
    return sorted(p.device for p in serial.tools.list_ports.comports() if p.vid == RPI_VID)


def wait_for(predicate, timeout: float, interval: float = 0.25):
    deadline = time.time() + timeout
    while time.time() < deadline:
        value = predicate()
        if value:
            return value
        time.sleep(interval)
    return None


def pick_port(explicit: str | None) -> str | None:
    if explicit:
        return explicit if explicit in find_tester_ports() or Path(explicit).exists() else None
    ports = find_tester_ports()
    if len(ports) > 1:
        print(f"  Multiple tester ports found ({', '.join(ports)}); pass --port.")
        return None
    return ports[0] if ports else None


# ── Flashing ──────────────────────────────────────────────────────────────────

def touch_1200(port: str):
    """arduino-pico reboots into BOOTSEL when the port is opened at 1200 baud and DTR drops."""
    try:
        s = serial.Serial(port, 1200)
        s.dtr = False
        s.close()
    except (serial.SerialException, OSError):
        pass  # the device often vanishes before close() completes


def flash_board(uf2: Path, port_arg: str | None, bootsel_arg: Path | None) -> bool:
    # Give a just-plugged board time to enumerate as either a drive or a serial port.
    wait_for(lambda: find_bootsel_drive(bootsel_arg) or find_tester_ports(), timeout=8)
    drive = find_bootsel_drive(bootsel_arg)
    if not drive:
        port = pick_port(port_arg)
        if not port:
            print("  No tester serial port and no BOOTSEL drive found.")
            return False
        print(f"  Rebooting {port} into BOOTSEL …")
        touch_1200(port)
        drive = wait_for(lambda: find_bootsel_drive(bootsel_arg), timeout=15)
        if not drive:
            print("  BOOTSEL drive did not appear. Hold BOOTSEL while plugging in and retry.")
            return False

    print(f"  Copying {uf2.name} → {drive} …")
    try:
        shutil.copyfile(uf2, drive / uf2.name)
    except OSError as e:
        # The RP2350 reboots as soon as the last block lands, so the final
        # flush/close can fail even though the image was written.
        print(f"  (copy finished with {type(e).__name__}: {e} — checking reboot)")

    if not wait_for(lambda: not find_bootsel_drive(bootsel_arg), timeout=20):
        print("  BOOTSEL drive did not unmount — flash probably failed.")
        return False
    return True


# ── Protocol ──────────────────────────────────────────────────────────────────

def query_hello(ser: "serial.Serial", timeout: float = 10) -> dict | None:
    """Sends HELLO repeatedly until the firmware answers (it may still be booting)."""
    deadline = time.time() + timeout
    while time.time() < deadline:
        ser.reset_input_buffer()
        ser.write(b"HELLO\n")
        ser.flush()
        attempt_end = min(deadline, time.time() + 1.5)
        while time.time() < attempt_end:
            line = ser.readline().decode("ascii", errors="replace").strip()
            if line.startswith("HELLO "):
                return dict(t.partition("=")[::2] for t in line[6:].split() if "=" in t)
    return None


def query_adapter_retry(ser: "serial.Serial", timeout: float = 8) -> tuple[dict | None, str | None]:
    """The tester polls for adapters every 2 s after boot, so NO_ADAPTER right after
    a reboot is not final."""
    deadline = time.time() + timeout
    info, err = None, None
    while time.time() < deadline:
        info, err = query_adapter(ser, timeout=2)
        if info or (err and "ADAPTER_NOT_PROVISIONED" in err):
            return info, err
        time.sleep(0.5)
    return info, err


def provision_adapter(ser: "serial.Serial", hw: int, padmaps: list[int], lifespan: int,
                      date_arg: str | None) -> bool:
    info, err = query_adapter_retry(ser)
    if info:
        print("  Adapter already provisioned — re-provisioning, counters preserved:")
        print_adapter_info(info)
        ins = int(info.get("ins", "0"))
        tests = int(info.get("tests", "0"))
        eol = 0 if lifespan > ins else (1 if info.get("eol") == "1" else 0)
        existing_date = info.get("mfg_date", "0")
        mfg_date = date_arg or (existing_date if existing_date != "0" else date.today().strftime("%Y%m%d"))
    elif err and "ADAPTER_NOT_PROVISIONED" in err:
        print("  Adapter blank — provisioning.")
        ins = tests = eol = None
        mfg_date = date_arg or date.today().strftime("%Y%m%d")
    else:
        print(f"  No adapter: {parse_error_line(err) if err else 'no response'}")
        return False

    return send_provision(ser, hw, padmaps, lifespan, mfg_date, ins=ins, tests=tests, eol=eol)


# ── Database ──────────────────────────────────────────────────────────────────

def load_db(path: Path) -> list[dict]:
    if not path.exists():
        return []
    with path.open(newline="") as f:
        return list(csv.DictReader(f))


def append_db(path: Path, row: dict):
    is_new = not path.exists()
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("a", newline="") as f:
        w = csv.DictWriter(f, fieldnames=DB_FIELDS)
        if is_new:
            w.writeheader()
        w.writerow(row)


def warn_duplicates(rows: list[dict], tester_uid: str, adapter_uid: str):
    for key, uid, label in (("tester_uid", tester_uid, "Tester"), ("adapter_uid", adapter_uid, "Adapter")):
        if not uid:
            continue
        prev = [r for r in rows if r.get(key) == uid]
        if prev:
            print(f"  NOTE: {label} {uid} already logged {len(prev)}× (last {prev[-1]['timestamp']}).")


# ── Per-board flow ────────────────────────────────────────────────────────────

def setup_one(args, padmaps: list[int], uf2: Path | None, expected_build: str | None,
              db_rows: list[dict]) -> bool:
    flashed = False
    if uf2:
        if not flash_board(uf2, args.port, args.bootsel):
            return False
        flashed = True

    port = wait_for(lambda: pick_port(args.port), timeout=20)
    if not port:
        print("  Tester serial port not found.")
        return False

    print(f"  Connecting to {port} …")
    try:
        ser = serial.Serial(port, args.baud, timeout=0.5)
    except serial.SerialException as e:
        print(f"  Could not open {port}: {e}")
        return False

    with ser:
        hello = query_hello(ser)
        if not hello:
            print("  No HELLO response from tester.")
            return False
        tester_uid = hello.get("uid", "")
        fw_build = hello.get("build", "")
        print(f"  Tester UID : {tester_uid}")
        print(f"  Firmware   : {fw_build} (proto {hello.get('proto', '1')})")
        if flashed and expected_build and fw_build != expected_build:
            print(f"  ERROR: expected build {expected_build} after flashing, tester reports {fw_build}.")
            return False

        provisioned = False
        if not args.no_provision:
            if not provision_adapter(ser, args.hw, padmaps, args.lifespan, args.date):
                return False
            provisioned = True

        info, err = query_adapter_retry(ser)
        if info:
            print("  Adapter:")
            print_adapter_info(info)
        elif not args.no_provision:
            print(f"  Could not read back adapter: {parse_error_line(err) if err else 'no response'}")
            return False
        else:
            print(f"  Adapter: {parse_error_line(err) if err else 'not detected'} — logging tester only.")

    info = info or {}
    if provisioned:
        expected_pm = ",".join(str(p) for p in padmaps)
        if info.get("ahw") != str(args.hw) or info.get("pm") != expected_pm:
            print(f"  ERROR: read-back mismatch (ahw={info.get('ahw')} pm={info.get('pm')}, "
                  f"expected ahw={args.hw} pm={expected_pm}).")
            return False

    adapter_uid = info.get("aid", "")
    if adapter_uid == "0" * 16:
        print("  WARNING: adapter serial read failed (aid is all zeros).")

    warn_duplicates(db_rows, tester_uid, adapter_uid)

    note = args.note or ""
    if not args.yes:
        try:
            extra = input("  Note for this board (Enter to skip): ").strip()
        except (KeyboardInterrupt, EOFError):
            extra = ""
        note = "; ".join(filter(None, [note, extra]))

    row = {
        "timestamp": datetime.now().isoformat(timespec="seconds"),
        "tester_uid": tester_uid,
        "fw_build": fw_build,
        "proto": hello.get("proto", "1"),
        "adapter_uid": adapter_uid,
        "adapter_hw": info.get("ahw", ""),
        "padmaps": info.get("pm", ""),
        "lifespan": info.get("lifespan", ""),
        "mfg_date": info.get("mfg_date", ""),
        "ins": info.get("ins", ""),
        "tests": info.get("tests", ""),
        "flashed": int(flashed),
        "provisioned": int(provisioned),
        "note": note,
    }
    append_db(args.db, row)
    db_rows.append(row)
    print(f"  Logged to {args.db}")
    return True


def main():
    ap = argparse.ArgumentParser(description="Flash, provision and log BondTest72 boards")
    ap.add_argument("--hw", type=int, default=None,
                    help=f"Adapter hardware ID ({', '.join(f'{k}={v}' for k, v in ADAPTER_HW_NAMES.items())})")
    ap.add_argument("--padmap", default=None,
                    help=f"Pad map IDs, comma-separated ({', '.join(f'{k}={v.split()[0]}' for k, v in PAD_MAPS.items())})")
    ap.add_argument("--lifespan", type=int, default=9999, help="Max insertions before EOL (default: 9999)")
    ap.add_argument("--date", default=None,
                    help="Manufacture date YYYYMMDD (default: keep existing, else today)")
    ap.add_argument("--db", type=Path, default=DEFAULT_DB, help=f"CSV database (default: {DEFAULT_DB})")
    ap.add_argument("--uf2", type=Path, default=None, help="Flash this UF2 instead of building")
    ap.add_argument("--no-flash", action="store_true", help="Skip building and flashing")
    ap.add_argument("--no-provision", action="store_true", help="Skip adapter provisioning, only log UIDs")
    ap.add_argument("--port", default=None, help="Serial port (default: auto-detect by Raspberry Pi USB VID)")
    ap.add_argument("--baud", type=int, default=115200)
    ap.add_argument("--bootsel", type=Path, default=None,
                    help="BOOTSEL drive mount point (default: auto, e.g. /Volumes/RP2350)")
    ap.add_argument("--note", default=None, help="Note added to every logged row")
    ap.add_argument("--once", action="store_true", help="Set up a single board and exit")
    ap.add_argument("--yes", "-y", action="store_true",
                    help="Don't wait for Enter between boards or prompt for notes")
    args = ap.parse_args()

    padmaps: list[int] = []
    if not args.no_provision:
        if args.hw is None or args.padmap is None:
            ap.error("--hw and --padmap are required unless --no-provision is given")
        if args.hw not in ADAPTER_HW_NAMES:
            ap.error(f"unknown hw {args.hw}. Known: {list(ADAPTER_HW_NAMES)}")
        try:
            padmaps = [int(p) for p in args.padmap.split(",")]
        except ValueError:
            ap.error(f"--padmap must be comma-separated integers, got '{args.padmap}'")
        if not 1 <= len(padmaps) <= 4 or any(p not in PAD_MAPS for p in padmaps):
            ap.error(f"--padmap must be 1–4 IDs from {list(PAD_MAPS)}")
    if args.date and (len(args.date) != 8 or not args.date.isdigit()):
        ap.error(f"--date must be YYYYMMDD, got '{args.date}'")

    uf2: Path | None = None
    expected_build: str | None = None
    if not args.no_flash:
        if args.uf2:
            uf2 = args.uf2
        else:
            if not build_firmware():
                print("ERROR: firmware build failed.")
                sys.exit(1)
            uf2 = DEFAULT_UF2
            expected_build = read_build_id()
        if not uf2.exists():
            print(f"ERROR: UF2 not found: {uf2}")
            sys.exit(1)

    print()
    print(f"Firmware : {uf2 or 'not flashing'}" + (f" (build {expected_build})" if expected_build else ""))
    if args.no_provision:
        print("Adapter  : not provisioning")
    else:
        print(f"Adapter  : hw {args.hw} ({ADAPTER_HW_NAMES[args.hw]}), padmap {args.padmap}, "
              f"lifespan {args.lifespan}")
    print(f"Database : {args.db}")

    db_rows = load_db(args.db)
    ok_count = fail_count = 0
    board_no = 1
    while True:
        print(f"\n── Board {board_no} " + "─" * 60)
        if not args.yes:
            try:
                answer = input("Connect tester + adapter, then press Enter (q to quit): ").strip().lower()
            except (KeyboardInterrupt, EOFError):
                answer = "q"
            if answer == "q":
                break

        try:
            ok = setup_one(args, padmaps, uf2, expected_build, db_rows)
        except KeyboardInterrupt:
            print("\n  Interrupted.")
            ok = False
            if args.yes or args.once:
                break
        if ok:
            ok_count += 1
            print("  ✔ DONE — disconnect the board.")
        else:
            fail_count += 1
            print("  ✘ FAILED — board not logged.")

        board_no += 1
        if args.once or args.yes and not ok:
            break
        if args.yes:
            # Wait for this board to go away and the next one to appear.
            print("  Waiting for board to be disconnected …")
            wait_for(lambda: not find_tester_ports() and not find_bootsel_drive(args.bootsel), timeout=3600)
            print("  Waiting for next board …")
            wait_for(lambda: find_tester_ports() or find_bootsel_drive(args.bootsel), timeout=3600)
            time.sleep(1.0)

    print(f"\n{ok_count} board(s) set up, {fail_count} failed. Database: {args.db}")
    sys.exit(0 if fail_count == 0 else 1)


if __name__ == "__main__":
    main()

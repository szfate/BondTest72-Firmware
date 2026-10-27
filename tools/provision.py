#!/usr/bin/env -S uv run
# /// script
# requires-python = ">=3.11"
# dependencies = [
#   "pyserial",
# ]
# ///
"""
provision.py — read/write adapter EEPROM on BondTest72

Queries the current adapter state, then (if --hw/--padmap given) writes hardware ID,
padmap IDs, lifespan, and manufacture date. With just --port, only reads and displays
the current EEPROM contents — no write.

Usage:
    uv run tools/provision.py --port /dev/tty.usbmodem11101              # read-only
    uv run tools/provision.py --port /dev/tty.usbmodem11101 --hw 1 --padmap 1
    uv run tools/provision.py --port COM3 --hw 1 --padmap 2 --date 20260101 --yes
    uv run tools/provision.py --port /dev/tty.usbmodem1101 --hw 1 --padmap 2,3 --lifespan 500
"""

import argparse
import sys
import time
from datetime import date

try:
    import serial
except ImportError:
    print("ERROR: pyserial not installed.")
    sys.exit(1)

# Known pad maps (mirrors pad_map_registry.cpp)
PAD_MAPS = {
    1: "1x1 v1 COB (63 cases — die44 unconnected)",
    2: "1x1 v2 COB (64 cases — die44 connected)",
    3: "1x0.5  (64 cases — 1×0.5 die)",
    4: "G801-MOSB (69 cases — unique pinout, single GND, no bypass caps)",
    5: "0p5x1 (64 cases — 0.5×1 die)",
    6: "G801-TQVA (41 cases — 37 IO + 4 VDD w/ caps, 6 GND)",
}

ADAPTER_HW_NAMES = {
    1: "Mezzanine70 r1",
    2: "Mezzanine70 r2",
}

DEFAULT_LIFESPAN = 250  # insertions, for a new record; re-provisioning keeps the adapter's


def parse_adapter_line(line: str) -> dict:
    """Parse 'ADAPTER aid=X ahw=N pm=N[,N] ... mfg_date=N ins=N tests=N eol=N dut=N'"""
    info = {}
    for token in line.split():
        if "=" in token:
            k, _, v = token.partition("=")
            info[k] = v
    return info


def parse_error_line(line: str) -> str:
    """Parse 'ERROR code=N msg=TEXT' or legacy 'ERROR text'. Returns human-readable string."""
    if line.startswith("ERROR "):
        rest = line[6:]
        if "=" in rest:
            parts = {}
            for token in rest.split():
                if "=" in token:
                    k, _, v = token.partition("=")
                    parts[k] = v
            code = parts.get("code", "?")
            msg = parts.get("msg", rest)
            return f"code {code}: {msg}"
        return rest
    return line


def query_adapter(ser: "serial.Serial", timeout: int = 5) -> tuple[dict | None, str | None]:
    """Returns (info, error_line). error_line is the raw 'ERROR ...' line (for
    the caller to classify, e.g. ADAPTER_NOT_PROVISIONED vs NO_ADAPTER), or
    None on success/timeout."""
    ser.reset_input_buffer()
    ser.write(b"GET_ADAPTER\n")
    ser.flush()
    start = time.time()
    while time.time() - start < timeout:
        line = ser.readline().decode("ascii", errors="replace").strip()
        if line.startswith("ADAPTER "):
            return parse_adapter_line(line[8:]), None
        if line.startswith("ERROR"):
            return None, line
    return None, None


def query_adapter_retry(ser: "serial.Serial", timeout: float = 8) -> tuple[dict | None, str | None]:
    """The tester polls for adapters every 1.5 s after boot, so NO_ADAPTER right after
    a reboot is not final."""
    deadline = time.time() + timeout
    info, err = None, None
    while time.time() < deadline:
        info, err = query_adapter(ser, timeout=2)
        if info or (err and "ADAPTER_NOT_PROVISIONED" in err):
            return info, err
        time.sleep(0.5)
    return info, err


def lifespan_cut_error(lifespan: int, info: dict) -> str | None:
    """Error text if `lifespan` lowers the adapter's lifespan to its insertion count
    or below. With the counters kept, that only retires the adapter at its next
    insertion, which is never the point of re-provisioning."""
    ins = int(info.get("ins", "0"))
    existing = info.get("lifespan", "")
    if lifespan > ins or (existing.isdigit() and lifespan >= int(existing)):
        return None
    return (f"lifespan {lifespan} is not above the adapter's {ins} insertions "
            f"(current lifespan {existing or '?'}); it would reach end-of-life at the next insertion.")


def send_provision(ser: "serial.Serial", hw_id: int, padmap_ids: list[int], lifespan: int, mfg_date: str,
                   ins: int | None = None, tests: int | None = None, eol: int | None = None,
                   timeout: int = 5) -> bool:
    ser.reset_input_buffer()
    pm_str = ','.join(str(p) for p in padmap_ids)
    cmd = f"PROVISION hw={hw_id} padmap={pm_str} lifespan={lifespan} date={mfg_date}"
    if ins is not None:
        cmd += f" ins={ins}"
    if tests is not None:
        cmd += f" tests={tests}"
    if eol is not None:
        cmd += f" eol={eol}"
    cmd += "\n"
    ser.write(cmd.encode())
    ser.flush()
    start = time.time()
    while time.time() - start < timeout:
        line = ser.readline().decode("ascii", errors="replace").strip()
        if line == "OK PROVISION":
            return True
        if line.startswith("ERROR"):
            print(f"  Tester error: {parse_error_line(line)}")
            return False
    print("  Timed out waiting for OK PROVISION.")
    return False


def fmt_date(d: str) -> str:
    if not d or d == "0":
        return "not set"
    try:
        return f"{d[:4]}-{d[4:6]}-{d[6:8]}"
    except Exception:
        return d


def print_adapter_info(info: dict):
    uid      = info.get("aid",      "?")
    hw       = info.get("ahw",      "?")
    pm       = info.get("pm",       "")
    mfg      = fmt_date(info.get("mfg_date", "0"))
    ins      = info.get("ins",      "?")
    tests    = info.get("tests",    "?")
    lifespan = info.get("lifespan", "?")
    eol      = info.get("eol",      "?")
    dut      = info.get("dut",      "?")

    padmap_ids = [s for s in pm.split(',') if s] if pm else []

    hw_name = ADAPTER_HW_NAMES.get(int(hw), f"hw {hw}") if hw != "?" else f"hw {hw}"
    pm_display = ', '.join(PAD_MAPS.get(int(p), f"pm {p}") for p in padmap_ids) if padmap_ids else 'none'
    print(f"  UID        : {uid}")
    print(f"  Hardware   : {hw_name} (hw {hw})")
    print(f"  Pad map IDs: {pm} ({pm_display})")
    print(f"  Mfg date   : {mfg}")
    print(f"  Insertions : {ins} / {lifespan}")
    print(f"  Tests      : {tests}")
    print(f"  EOL        : {'YES' if eol == '1' else 'no'}")
    print(f"  DUT present : {'YES' if dut == '1' else 'no'}")


def main():
    ap = argparse.ArgumentParser(description="Provision BondTest72 adapter EEPROM")
    ap.add_argument("--port",    required=True,  help="Serial port")
    ap.add_argument("--baud",    type=int, default=115200)
    ap.add_argument("--hw",      type=int, default=None,
                    help=f"Hardware ID ({', '.join(f'{k}={v}' for k, v in ADAPTER_HW_NAMES.items())}). "
                         "Omit along with --padmap to just read and display current EEPROM contents.")
    ap.add_argument("--padmap",  default=None,
                    help=f"Pad map IDs, comma-separated ({', '.join(f'{k}={v.split()[0]}' for k, v in PAD_MAPS.items())})")
    ap.add_argument("--date",    default=None,
                    help="Manufacture date YYYYMMDD (default: keep the adapter's date; today for a blank adapter)")
    ap.add_argument("--lifespan", type=int, default=None,
                    help=f"Designed lifespan — max insertions before EOL (default: keep the adapter's; "
                         f"{DEFAULT_LIFESPAN} for a blank adapter)")
    ap.add_argument("--yes", "-y", action="store_true",
                    help="Skip confirmation prompt")
    ap.add_argument("--override", action="store_true",
                    help="Reset insertion/test counters and EOL flag to 0 instead of preserving existing values")
    args = ap.parse_args()

    read_only = args.hw is None and args.padmap is None
    if not read_only and (args.hw is None or args.padmap is None):
        print("ERROR: --hw and --padmap must be given together (or both omitted for a read-only query).")
        sys.exit(1)

    if not read_only:
        # Parse and validate padmap IDs
        try:
            padmap_ids = [int(p) for p in args.padmap.split(',')]
        except ValueError:
            print(f"ERROR: --padmap must be comma-separated integers, got '{args.padmap}'")
            sys.exit(1)

        if len(padmap_ids) > 4:
            print(f"ERROR: too many padmap IDs (max 4), got {len(padmap_ids)}")
            sys.exit(1)

        for p in padmap_ids:
            if p not in PAD_MAPS:
                print(f"ERROR: unknown padmap {p}. Known: {list(PAD_MAPS.keys())}")
                sys.exit(1)

        # Validate date format
        if args.date is not None and (len(args.date) != 8 or not args.date.isdigit()):
            print(f"ERROR: --date must be YYYYMMDD, got '{args.date}'")
            sys.exit(1)

        if args.hw not in ADAPTER_HW_NAMES:
            print(f"ERROR: unknown hw {args.hw}. Known: {list(ADAPTER_HW_NAMES.keys())}")
            sys.exit(1)

    print(f"Opening {args.port} at {args.baud} baud …")
    ser = serial.Serial(args.port, args.baud, timeout=2)
    time.sleep(0.5)

    # Query current state
    print("\nCurrent adapter:")
    info, err = query_adapter_retry(ser)
    blank = bool(err and "ADAPTER_NOT_PROVISIONED" in err)
    if info:
        print_adapter_info(info)
    elif blank:
        print("  Adapter present but not provisioned (blank EEPROM).")
        if read_only:
            print("  Run again with --hw and --padmap to provision it.")
    elif err:
        print(f"  Error: {parse_error_line(err)}")
    else:
        print("  (no adapter detected or not responding)")

    if read_only:
        ser.close()
        sys.exit(0 if info else 1)

    # PROVISION without counters sets them to 0. Only a blank adapter (or
    # --override) may get that: if the record couldn't be read, its counters are
    # unknown, and writing 0 would restart the adapter's end-of-life count.
    if not info and not blank and not args.override:
        print("\nERROR: couldn't read the adapter's current record, so its insertion/test counters")
        print("       can't be preserved. Re-seat the adapter or power-cycle the tester and try again,")
        print("       or pass --override to reset the counters to 0.")
        ser.close()
        sys.exit(1)

    # Keep the adapter's lifespan on re-provisioning unless --lifespan is given;
    # raising it above the insertion count is what clears the EOL flag below.
    if args.lifespan is not None:
        lifespan = args.lifespan
    elif info and info.get("lifespan", "").isdigit():
        lifespan = int(info["lifespan"])
    else:
        lifespan = DEFAULT_LIFESPAN

    if info and not args.override and (cut := lifespan_cut_error(lifespan, info)):
        print(f"\nERROR: {cut}")
        print("       Give a higher --lifespan, or --override to reset the counters as well.")
        ser.close()
        sys.exit(1)

    if args.override or not info:
        counter_ins = None
        counter_tests = None
        counter_eol = None
    else:
        counter_ins = int(info.get("ins", "0"))
        counter_tests = int(info.get("tests", "0"))
        counter_eol = 1 if info.get("eol", "0") == "1" else 0
        if lifespan > counter_ins:
            counter_eol = 0

    # Keep the adapter's manufacture date on re-provisioning; today is only for a
    # new (blank or unreadable) record.
    existing_date = info.get("mfg_date", "0") if info else "0"
    mfg_date = args.date or (existing_date if existing_date != "0" else date.today().strftime("%Y%m%d"))

    # Show what will be written
    pm_display = ', '.join(f'{p} — {PAD_MAPS[p]}' for p in padmap_ids)
    print(f"\nProvision with:")
    print(f"  Hardware : {args.hw} — {ADAPTER_HW_NAMES[args.hw]}")
    print(f"  Pad maps : {pm_display}")
    print(f"  Lifespan : {lifespan} insertions")
    print(f"  Mfg date : {fmt_date(mfg_date)}")
    if args.override:
        print("  Counters  : resetting to 0 (--override)")
    elif counter_ins is not None:
        eol_str = "yes" if counter_eol else "no"
        print(f"  Counters  : preserving (ins={counter_ins}, tests={counter_tests}, eol={eol_str})")
    else:
        print("  Counters  : 0 (blank adapter)")

    if not args.yes:
        try:
            answer = input("\nProceed? [y/N] ").strip().lower()
        except (KeyboardInterrupt, EOFError):
            print("\nAborted.")
            ser.close()
            sys.exit(0)
        if answer != "y":
            print("Aborted.")
            ser.close()
            sys.exit(0)

    print("\nProvisioning …")
    if not send_provision(ser, args.hw, padmap_ids, lifespan, mfg_date,
                          ins=counter_ins, tests=counter_tests, eol=counter_eol):
        ser.close()
        sys.exit(1)

    print("  OK.")

    # Read back to confirm
    print("\nAdapter after provisioning:")
    info, err = query_adapter(ser)
    if info:
        print_adapter_info(info)
    else:
        print("  (could not read back adapter info)")

    ser.close()


if __name__ == "__main__":
    main()

#!/usr/bin/env -S uv run
# /// script
# requires-python = ">=3.11"
# dependencies = [
#   "pyserial",
# ]
# ///
"""
provision.py — write adapter EEPROM on BondTest72

Queries the current adapter state, then writes hardware ID, padmap IDs, lifespan, and manufacture date.

Usage:
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
    1: "Mezzanine70 v1  (63 cases — die44 unconnected)",
    2: "Mezzanine70 v2  (64 cases — die44 connected)",
    3: "Mezzanine70 v1 1x0p5  (64 cases — 1×0.5 die)",
}

ADAPTER_HW_NAMES = {
    1: "Mezzanine70",
}


def parse_adapter_line(line: str) -> dict:
    """Parse 'ADAPTER hw=N pm=N[,N] ... mfg_date=N ins=N tests=N eol=N dut=N'"""
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


def query_adapter(ser: "serial.Serial", timeout: int = 5) -> dict | None:
    ser.reset_input_buffer()
    ser.write(b"GET_ADAPTER\n")
    ser.flush()
    start = time.time()
    while time.time() - start < timeout:
        line = ser.readline().decode("ascii", errors="replace").strip()
        if line.startswith("ADAPTER "):
            return parse_adapter_line(line[8:])
        if line.startswith("ERROR"):
            print(f"  Error: {parse_error_line(line)}")
            return None
    return None


def send_provision(ser: "serial.Serial", hw_id: int, padmap_ids: list[int], lifespan: int, mfg_date: str, timeout: int = 5) -> bool:
    ser.reset_input_buffer()
    pm_str = ','.join(str(p) for p in padmap_ids)
    cmd = f"PROVISION hw={hw_id} padmap={pm_str} lifespan={lifespan} date={mfg_date}\n"
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
    uid      = info.get("uid",      "?")
    hw       = info.get("hw",       "?")
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
    ap.add_argument("--hw",      type=int, required=True,
                    help=f"Hardware ID ({', '.join(f'{k}={v}' for k, v in ADAPTER_HW_NAMES.items())})")
    ap.add_argument("--padmap",  required=True,
                    help=f"Pad map IDs, comma-separated ({', '.join(f'{k}={v.split()[0]}' for k, v in PAD_MAPS.items())})")
    ap.add_argument("--date",    default=None,
                    help="Manufacture date YYYYMMDD (default: today)")
    ap.add_argument("--lifespan", type=int, default=250,
                    help="Designed lifespan — max insertions before EOL (default: 250)")
    ap.add_argument("--yes", "-y", action="store_true",
                    help="Skip confirmation prompt")
    args = ap.parse_args()

    mfg_date = args.date or date.today().strftime("%Y%m%d")

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
    if len(mfg_date) != 8 or not mfg_date.isdigit():
        print(f"ERROR: --date must be YYYYMMDD, got '{mfg_date}'")
        sys.exit(1)

    if args.hw not in ADAPTER_HW_NAMES:
        print(f"ERROR: unknown hw {args.hw}. Known: {list(ADAPTER_HW_NAMES.keys())}")
        sys.exit(1)

    print(f"Opening {args.port} at {args.baud} baud …")
    ser = serial.Serial(args.port, args.baud, timeout=2)
    time.sleep(0.5)

    # Query current state
    print("\nCurrent adapter:")
    info = query_adapter(ser)
    if info:
        print_adapter_info(info)
    else:
        print("  (no adapter detected or not responding)")

    # Show what will be written
    pm_display = ', '.join(f'{p} — {PAD_MAPS[p]}' for p in padmap_ids)
    print(f"\nProvision with:")
    print(f"  Hardware : {args.hw} — {ADAPTER_HW_NAMES[args.hw]}")
    print(f"  Pad maps : {pm_display}")
    print(f"  Lifespan : {args.lifespan} insertions")
    print(f"  Mfg date : {fmt_date(mfg_date)}")

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
    if not send_provision(ser, args.hw, padmap_ids, args.lifespan, mfg_date):
        ser.close()
        sys.exit(1)

    print("  OK.")

    # Read back to confirm
    print("\nAdapter after provisioning:")
    info = query_adapter(ser)
    if info:
        print_adapter_info(info)
    else:
        print("  (could not read back adapter info)")

    ser.close()


if __name__ == "__main__":
    main()

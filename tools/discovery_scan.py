#!/usr/bin/env -S uv run
# /// script
# requires-python = ">=3.11"
# dependencies = [
#   "pyserial",
#   "openpyxl",
#   "Pillow",
# ]
# ///
"""
discovery_scan.py — BondTest72 discovery scan tool

Sends DISCOVERY_SCAN to the tester, collects sense voltages for every mez-pin
pair (70×70), then writes three outputs:
  • .csv   — raw 74×74 voltage matrix
  • .xlsx  — same with color-scale conditional formatting
  • .png   — pixel heatmap (green=conducting, red=isolated)

Rows = source pad (current injected via Bus::D)
Cols = sink pad   (tester GND return via Bus::B)
Cell = sense voltage at Bus::D (ADC0) in volts, 3 d.p.

Usage:
    uv run tools/discovery_scan.py --port /dev/tty.usbmodem1101
    uv run tools/discovery_scan.py --port COM3 --out my_die_scan --cell 8
"""

import argparse
import colorsys
import csv
import sys
import time
from datetime import datetime

try:
    import serial
except ImportError:
    print("ERROR: pyserial not installed. Run: pip install pyserial")
    sys.exit(1)

# ── Mez-pin → die-pad mapping (derived from pad_map_registry.cpp, pm2) ───────
#
# GND mez pins (equivalent die GND pads): 10=die19, 18=die27, 26=die36,
#                                          46=die57, 53=die63, 61=die71
# Die pads with no mez connection (N/C):  9, 34, 46, 73

MEZ_TO_DIE = {
     1: 10,   2: 11,   3: 12,   4: 13,   5: 14,   6: 15,   7: 16,   8: 17,
     9: 18,                                                                    # VDDIO
    10: 19,                                                                    # GND
    11: 20,  12: 21,  13: 22,  14: 23,  15: 24,  16: 25,
    17: 26,                                                                    # PWR_AUX
    18: 27,                                                                    # GND
    19: 28,  20: 29,  21: 30,  22: 31,  23: 32,  24: 33,
    25: 35,                                                                    # VDD_CORE
    26: 36,                                                                    # GND
    27: 37,                                                                    # VDDIO
    28: 38,  29: 39,  30: 40,  31: 41,  32: 42,  33: 43,
    34: 44,                                                                    # IO (pm2)
    35: 45,
    36: 47,  37: 48,  38: 49,  39: 50,  40: 51,  41: 52,
    42: 53,  43: 54,  44: 55,  45: 56,
    46: 57,                                                                    # GND
    47: 58,                                                                    # VDDIO
    48: 59,  49: 60,  50: 61,  51: 62,
    52: 64,                                                                    # PWR_AUX
    53: 63,                                                                    # GND
    54: 65,  55: 66,  56: 67,  57: 68,  58: 69,  59: 70,
    60: 72,                                                                    # VDD_CORE
    61: 71,                                                                    # GND
    62: 74,                                                                    # VDDIO
    63:  1,  64:  2,  65:  3,  66:  4,  67:  5,  68:  6,  69:  7,  70:  8,
}

DIE_TO_MEZ   = {d: m for m, d in MEZ_TO_DIE.items()}
GND_MEZ_PINS = {10, 18, 26, 46, 53, 61}
NC_DIE_PADS  = {9, 34, 46, 73}

ALL_DIE_PADS = list(range(1, 75))   # 1..74

# Indices (0-based) of pads to label on axes: 1, 10, 20 … 70, 74
LABEL_INDICES = {i for i in range(74) if (i + 1) % 10 == 0 or i == 0 or i == 73}


def col_label(die: int) -> str:
    return f"P{die}"


# ── Serial scan ───────────────────────────────────────────────────────────────

def run_scan(port: str, baud: int, timeout: int) -> dict:
    """Return dict keyed (src_mez, snk_mez) → voltage float."""
    print(f"Opening {port} at {baud} baud …")
    ser = serial.Serial(port, baud, timeout=2)
    time.sleep(0.5)
    ser.reset_input_buffer()

    print("Sending DISCOVERY_SCAN …")
    ser.write(b"DISCOVERY_SCAN\n")
    ser.flush()

    data = {}
    start = time.time()
    total_expected = 70 * 69
    received = 0
    last_progress = -1

    while True:
        if time.time() - start > timeout:
            print(f"\nERROR: timed out after {timeout}s ({received}/{total_expected} received)")
            ser.close()
            sys.exit(1)

        line = ser.readline().decode("ascii", errors="replace").strip()
        if not line:
            continue

        if line == "DSCAN DONE":
            print(f"\rReceived {received}/{total_expected} measurements. Done.    ")
            break

        if line.startswith("DSCAN "):
            parts = line.split()
            if len(parts) >= 4:
                if '=' in parts[1]:
                    d = dict(p.split('=', 1) for p in parts[1:] if '=' in p)
                    src, snk, v = int(d.get('src', '0')), int(d.get('snk', '0')), float(d.get('sv', '0'))
                else:
                    src, snk, v = int(parts[1]), int(parts[2]), float(parts[3])
                data[(src, snk)] = v
                received += 1
                pct = received * 100 // total_expected
                if pct != last_progress:
                    print(f"\r  {pct:3d}%  ({received}/{total_expected})", end="", flush=True)
                    last_progress = pct
        elif line.startswith("ERROR "):
            rest = line[6:]
            if '=' in rest:
                kv = dict(p.split('=', 1) for p in rest.split() if '=' in p)
                print(f"\nERROR from tester (code {kv.get('code', '?')}): {kv.get('msg', rest)}")
            else:
                print(f"\nERROR from tester: {line}")
            ser.close()
            sys.exit(1)
        else:
            print(f"\n[tester] {line}", end="")

    ser.close()
    return data


# ── Matrix builder ────────────────────────────────────────────────────────────

def build_matrix(data: dict) -> list[list]:
    """Build 74×74 matrix (list of rows, each row is list of cell strings)."""
    rows = []
    for src_die in ALL_DIE_PADS:
        row = []
        for snk_die in ALL_DIE_PADS:
            if src_die == snk_die:
                row.append("")
                continue
            if src_die in NC_DIE_PADS or snk_die in NC_DIE_PADS:
                row.append("N/C")
                continue
            src_mez = DIE_TO_MEZ[src_die]
            snk_mez = DIE_TO_MEZ[snk_die]
            v = data.get((src_mez, snk_mez))
            row.append(f"{v:.3f}" if v is not None else "")
        rows.append(row)
    return rows


# ── CSV output ────────────────────────────────────────────────────────────────

def write_csv(matrix: list[list], path: str):
    header = ["src\\snk"] + [col_label(d) for d in ALL_DIE_PADS]
    with open(path, "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(header)
        for i, row in enumerate(matrix):
            w.writerow([col_label(ALL_DIE_PADS[i])] + row)
    print(f"CSV written → {path}")


# ── Excel output ──────────────────────────────────────────────────────────────

def write_excel(matrix: list[list], path: str):
    try:
        from openpyxl import Workbook
        from openpyxl.styles import Font, PatternFill, Alignment, Border, Side
        from openpyxl.formatting.rule import ColorScaleRule
    except ImportError:
        print("openpyxl not installed — skipping Excel output. Run: pip install openpyxl")
        return

    wb = Workbook()
    ws = wb.active
    ws.title = "Discovery Scan"

    header_fill = PatternFill("solid", fgColor="D0D0D0")
    header_font = Font(bold=True, size=8)
    cell_font   = Font(size=8)
    thin        = Side(style="thin", color="CCCCCC")
    border      = Border(left=thin, right=thin, top=thin, bottom=thin)
    center      = Alignment(horizontal="center", vertical="center")
    labels      = [col_label(d) for d in ALL_DIE_PADS]

    ws.cell(1, 1, "src\\snk").font = header_font
    ws.cell(1, 1).fill = header_fill
    ws.cell(1, 1).alignment = center

    for col_idx, lbl in enumerate(labels, start=2):
        c = ws.cell(1, col_idx, lbl)
        c.font = header_font
        c.fill = header_fill
        c.alignment = Alignment(horizontal="center", vertical="center", wrap_text=True)
        c.border = border
        ws.column_dimensions[c.column_letter].width = 3

    for row_idx, (src_die, row) in enumerate(zip(ALL_DIE_PADS, matrix), start=2):
        lc = ws.cell(row_idx, 1, labels[row_idx - 2])
        lc.font = header_font
        lc.fill = header_fill
        lc.alignment = center
        lc.border = border

        for col_idx, val in enumerate(row, start=2):
            c = ws.cell(row_idx, col_idx, val)
            c.font = cell_font
            c.alignment = center
            c.border = border
            if val and val != "N/C":
                try:
                    c.value = float(val)
                except ValueError:
                    pass

    data_range = f"B2:{ws.cell(75, 75).coordinate}"
    ws.conditional_formatting.add(
        data_range,
        ColorScaleRule(
            start_type="num", start_value=0.0,  start_color="63BE7B",
            mid_type="num",   mid_value=1.65,   mid_color="FFEB84",
            end_type="num",   end_value=3.3,    end_color="F8696B",
        )
    )

    ws.row_dimensions[1].height = 40
    ws.column_dimensions["A"].width = 3
    ws.freeze_panes = "B2"

    wb.save(path)
    print(f"Excel written → {path}")


# ── PNG heatmap ───────────────────────────────────────────────────────────────

def _voltage_to_color(val: str, is_diag: bool) -> tuple:
    """Map a cell value string to an RGB tuple."""
    if is_diag:
        return (200, 200, 200)
    if not val or val == "":
        return (220, 220, 220)
    if val == "N/C":
        return (210, 210, 210)
    try:
        v = float(val)
    except ValueError:
        return (70, 70, 70)
    v = max(0.0, min(3.3, v))
    # HSV: hue 120° (green) → 0° (red) across 0–3.3 V
    hue = (1.0 - v / 3.3) * 120.0 / 360.0
    r, g, b = colorsys.hsv_to_rgb(hue, 0.85, 0.88)
    return (int(r * 255), int(g * 255), int(b * 255))


def write_image(matrix: list[list], path: str, cell: int = 5):
    try:
        from PIL import Image, ImageDraw, ImageFont
    except ImportError:
        print("Pillow not installed — skipping PNG output. Run: pip install Pillow")
        return

    MARGIN = 26
    N      = 74
    BG     = (255, 255, 255)

    img  = Image.new("RGB", (MARGIN + N * cell, MARGIN + N * cell), BG)
    draw = ImageDraw.Draw(img)
    font = ImageFont.load_default()

    # Data cells
    for ri, row in enumerate(matrix):
        for ci, val in enumerate(row):
            x = MARGIN + ci * cell
            y = MARGIN + ri * cell
            draw.rectangle([x, y, x + cell - 1, y + cell - 1],
                           fill=_voltage_to_color(val, ri == ci))

    # Row labels (left margin) — every 10 pads plus pad 1 and 74
    for i, die in enumerate(ALL_DIE_PADS):
        if i in LABEL_INDICES:
            y = MARGIN + i * cell + cell // 2 - 4
            draw.text((1, y), str(die), fill=(80, 80, 80), font=font)

    # Column labels (top margin) — same positions, rotated 90°
    for i, die in enumerate(ALL_DIE_PADS):
        if i in LABEL_INDICES:
            x     = MARGIN + i * cell + cell // 2
            label = str(die)
            tw    = len(label) * 6
            tmp   = Image.new("RGB", (tw + 2, 10), BG)
            ImageDraw.Draw(tmp).text((0, 0), label, fill=(80, 80, 80), font=font)
            tmp   = tmp.rotate(90, expand=True)
            img.paste(tmp, (x - 4, 1))

    img.save(path)
    print(f"PNG written  → {path}")


# ── Entry point ───────────────────────────────────────────────────────────────

def main():
    ap = argparse.ArgumentParser(
        description="BondTest72 discovery scan — sweeps all pad pairs and saves voltage matrix"
    )
    ap.add_argument("--port",    required=True, help="Serial port (e.g. /dev/tty.usbmodem1101 or COM3)")
    ap.add_argument("--baud",    type=int, default=115200,  help="Baud rate (default: 115200)")
    ap.add_argument("--out",     default=None,              help="Output basename without extension (overrides --name)")
    ap.add_argument("--name",    default=None,              help="Label appended to auto-generated filename (e.g. TTPG)")
    ap.add_argument("--timeout", type=int, default=120,     help="Scan timeout in seconds (default: 120)")
    ap.add_argument("--cell",    type=int, default=16,      help="Heatmap cell size in pixels (default: 16)")
    args = ap.parse_args()

    ts       = datetime.now().strftime("%Y%m%d_%H%M%S")
    suffix   = f"_{args.name}" if args.name else ""
    out_base = args.out or f"discovery_{ts}{suffix}"

    data   = run_scan(args.port, args.baud, args.timeout)
    matrix = build_matrix(data)

    write_csv(matrix,              out_base + ".csv")
    write_excel(matrix,            out_base + ".xlsx")
    write_image(matrix, out_base + ".png", args.cell)

    print(f"\nDone. {len(data)} / {70 * 69} measurements.")


if __name__ == "__main__":
    main()

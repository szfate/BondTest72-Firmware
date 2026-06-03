#!/usr/bin/env -S uv run
# /// script
# requires-python = ">=3.11"
# dependencies = [
#   "PySide6",
#   "pyserial",
# ]
# ///
"""
live_serial_viewer.py — Live die map viewer that reads from serial port.

Displays per-pad bond results in real-time as the tester runs. Pads light up
as PAD results arrive. The die map and results table update incrementally.

Usage:
    uv run tools/live_serial_viewer.py --port /dev/tty.usbmodem1101
    uv run tools/live_serial_viewer.py --port COM3 --baud 115200
"""

import argparse
import sys
import os
import threading

sys.path.insert(0, os.path.dirname(__file__))

from die_visualizer.die_shapes import shape_1x1
from die_visualizer.die_map_widget import DieMapWidget, build_results_table, create_main_layout
from die_visualizer.result_types import BondResult, PadResult, SlotResult, TestResult
from die_visualizer.log_parser import parse_log

import serial
from PySide6.QtWidgets import QApplication, QMainWindow, QStatusBar, QVBoxLayout, QWidget
from PySide6.QtCore import Qt, QThread, Signal, QObject

PADMAP_SHAPES = {
    1: shape_1x1,
    2: shape_1x1,
}


class SerialReader(QThread):
    line_received = Signal(str)
    connection_lost = Signal()

    def __init__(self, port, baud, parent=None):
        super().__init__(parent)
        self.port = port
        self.baud = baud
        self._running = True

    def run(self):
        try:
            ser = serial.Serial(self.port, self.baud, timeout=1)
        except Exception as e:
            self.line_received.emit(f"__ERROR__ {e}")
            return

        buf = b""
        while self._running:
            try:
                data = ser.read(256)
                if not data:
                    continue
                buf += data
                while b"\n" in buf:
                    line, buf = buf.split(b"\n", 1)
                    try:
                        text = line.decode("utf-8", errors="replace").strip()
                    except Exception:
                        text = line.decode("latin-1", errors="replace").strip()
                    if text:
                        self.line_received.emit(text)
            except serial.SerialException:
                self.connection_lost.emit()
                break
            except OSError:
                self.connection_lost.emit()
                break
        try:
            ser.close()
        except Exception:
            pass

    def stop(self):
        self._running = False
        self.wait(2000)


class LiveViewer(QMainWindow):
    def __init__(self, shape, port, baud):
        super().__init__()
        self.shape = shape
        self.port_name = port
        self.results_by_die_pad = {}
        self.pads = []
        self.slots = []
        self.slot_index = {}
        self.outcome = None
        self.good_count = 0
        self.tested_count = 0

        self.setWindowTitle(f"BondTest72 — {shape.name} — Live Viewer")

        central = QWidget()
        self.setCentralWidget(central)
        layout = QVBoxLayout(central)

        self.die_map = DieMapWidget(shape=shape, results=self.results_by_die_pad, result_text="", parent=central)
        self.table = build_results_table(shape, self.results_by_die_pad)

        from PySide6.QtWidgets import QSplitter
        self.splitter = QSplitter(Qt.Orientation.Horizontal)
        self.splitter.addWidget(self.die_map)
        self.splitter.addWidget(self.table)
        self.splitter.setStretchFactor(0, 3)
        self.splitter.setStretchFactor(1, 2)
        layout.addWidget(self.splitter)

        self.status_bar = QStatusBar()
        self.setStatusBar(self.status_bar)
        self.status_bar.showMessage(f"Connecting to {port}...")

        self.resize(1200, 800)

        self.reader = SerialReader(port, baud)
        self.reader.line_received.connect(self._on_line)
        self.reader.connection_lost.connect(self._on_disconnect)
        self.reader.start()

    def _on_line(self, line):
        if line.startswith("PAD "):
            self._handle_pad(line)
        elif line.startswith("SLOT "):
            self._handle_slot(line)
        elif line.startswith("SUMMARY "):
            self._handle_summary(line)
        elif line.startswith("EVENT "):
            self.status_bar.showMessage(line)
        elif line.startswith("__ERROR__ "):
            self.status_bar.showMessage(f"Error: {line[10:]}")
        elif line.startswith("ERROR "):
            self.status_bar.showMessage(line)

    def _handle_pad(self, line):
        from die_visualizer.log_parser import _detect_format, _parse_pad_kv, _parse_pad_positional
        parts = line.split()
        fmt = _detect_format(line)
        parsed = _parse_pad_kv(parts) if fmt == 'kv' else _parse_pad_positional(parts)
        if parsed:
            slot_num, pr = parsed
            if self.shape:
                try:
                    info = self.shape.pad_by_die_pad(pr.die_pad)
                    pr.role = info.role
                except ValueError:
                    pass
            self.results_by_die_pad[pr.die_pad] = pr
            self.pads.append(pr)
            self.slot_index.setdefault(slot_num, []).append(pr)
            self.die_map.results = self.results_by_die_pad
            tested = len(self.results_by_die_pad)
            good = sum(1 for p in self.results_by_die_pad.values() if p.bond == BondResult.GOOD)
            self.die_map.result_text = f"{good}/{tested}"
            self.die_map.update()
            self.status_bar.showMessage(f"{self.port_name} — {tested} pads, {good} good")

    def _handle_slot(self, line):
        from die_visualizer.log_parser import _detect_format, _parse_slot_kv, _parse_slot_positional
        parts = line.split()
        fmt = _detect_format(line)
        sr = _parse_slot_kv(parts) if fmt == 'kv' else _parse_slot_positional(parts)
        if sr:
            sr.pads = self.slot_index.get(sr.slot, [])
            self.slots.append(sr)

    def _handle_summary(self, line):
        from die_visualizer.log_parser import _detect_format, _parse_summary_kv, _parse_summary_positional
        parts = line.split()
        fmt = _detect_format(line)
        result = _parse_summary_kv(parts) if fmt == 'kv' else _parse_summary_positional(parts)
        if result:
            self.outcome, self.good_count, self.tested_count = result
            self.die_map.result_text = f"{self.good_count}/{self.tested_count} {self.outcome}"
            self.die_map.update()
            self.status_bar.showMessage(f"{self.outcome} {self.good_count}/{self.tested_count}")
            self.setWindowTitle(
                f"BondTest72 — {self.shape.name} — {self.outcome} {self.good_count}/{self.tested_count}"
            )
            self._refresh_table()

    def _refresh_table(self):
        new_table = build_results_table(self.shape, self.results_by_die_pad)
        if self.splitter.count() >= 2:
            old = self.splitter.replaceWidget(1, new_table)
            if old:
                old.deleteLater()
        else:
            self.splitter.addWidget(new_table)
        self.splitter.setStretchFactor(0, 3)
        self.splitter.setStretchFactor(1, 2)
        self.table = new_table

    def _on_disconnect(self):
        self.status_bar.showMessage("Connection lost")

    def closeEvent(self, event):
        self.reader.stop()
        super().closeEvent(event)


def main():
    parser = argparse.ArgumentParser(description="BondTest72 live serial viewer")
    parser.add_argument("--port", required=True, help="Serial port (e.g. /dev/tty.usbmodem1101 or COM3)")
    parser.add_argument("--baud", type=int, default=115200, help="Baud rate (default: 115200)")
    parser.add_argument("--padmap", type=int, default=2, help="Pad map ID (default: 2)")
    args = parser.parse_args()

    shape = PADMAP_SHAPES.get(args.padmap)
    if shape is None:
        print(f"Error: padmap {args.padmap} not yet supported")
        sys.exit(1)

    app = QApplication(sys.argv)
    window = LiveViewer(shape, args.port, args.baud)
    window.show()
    ret = app.exec()
    window.reader.stop()
    sys.exit(ret)


if __name__ == "__main__":
    main()
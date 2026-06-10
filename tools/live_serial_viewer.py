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

sys.path.insert(0, os.path.dirname(__file__))

from die_visualizer.die_shapes import shape_1x1, shape_1x0p5
from die_visualizer.die_map_widget import DieMapWidget, build_results_table
from die_visualizer.result_types import BondResult, PadResult, SlotResult

import serial
from PySide6.QtWidgets import (
    QApplication, QMainWindow, QStatusBar, QVBoxLayout, QWidget,
    QFrame, QHBoxLayout, QLabel, QSplitter, QSizePolicy, QPushButton,
)
from PySide6.QtCore import Qt, QThread, Signal, QObject

# Human-readable adapter model names from firmware enum
ADAPTER_HW_NAMES = {
    "1": "Mezzanine70",
}

PADMAP_NAMES = {
    "1": "1x1 v1",
    "2": "1x1 v2",
    "3": "1x0p5",
}

PADMAP_SHAPES = {
    1: shape_1x1,
    2: shape_1x1,
    3: shape_1x0p5,
}




class SerialReader(QThread):
    line_received = Signal(str)
    connection_lost = Signal()
    hello_received = Signal(str)  # raw HELLO kv string
    adapter_received = Signal(str)  # raw ADAPTER kv string

    def __init__(self, port, baud, parent=None):
        super().__init__(parent)
        self.port = port
        self.baud = baud
        self._running = True
        self._ser = None

    def run(self):
        try:
            self._ser = serial.Serial(self.port, self.baud, timeout=0.05)
        except Exception as e:
            self.line_received.emit(f"__ERROR__ {e}")
            return

        self._ser.write(b"HELLO\n")
        self._ser.write(b"GET_ADAPTER\n")

        while self._running:
            try:
                line = self._ser.read_until(b"\n")
                if not line:
                    continue
                try:
                    text = line.decode("utf-8", errors="replace").strip()
                except Exception:
                    text = line.decode("latin-1", errors="replace").strip()
                if not text:
                    continue
                if text.startswith("HELLO "):
                    self.hello_received.emit(text[6:])
                elif text.startswith("ADAPTER "):
                    self.adapter_received.emit(text)
                else:
                    self.line_received.emit(text)
            except serial.SerialException:
                self.connection_lost.emit()
                break
            except OSError:
                self.connection_lost.emit()
                break
        try:
            if self._ser:
                self._ser.close()
            self._ser = None
        except Exception:
            pass

    def request_adapter(self):
        if self._ser is not None and self._ser.is_open:
            try:
                self._ser.write(b"GET_ADAPTER\n")
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

        self._tester_name = ""
        self._tester_build = ""
        self._tester_uid = ""
        self._adapter_padmap_id = ""
        self._adapter_hw_name = ""
        self._adapter_uid = ""
        self._adapter_ins = 0
        self._adapter_tests = 0
        self._adapter_eol = False
        self._adapter_supported = []
        self._adapter_supported_ids = []
        self._adapter_dut_present = False

        self.shape = shape or shape_1x1
        self.setWindowTitle(f"BondTest72 — {self.shape.name} — Live Viewer")

        central = QWidget()
        self.setCentralWidget(central)
        self._central = central
        layout = QVBoxLayout(central)
        layout.setSpacing(0)
        layout.setContentsMargins(0, 0, 0, 0)

        # Top bar for tester / adapter info
        self.top_bar = QFrame()
        self.top_bar.setStyleSheet("""
            QFrame {
                background-color: #1e1e2f;
            }
            QLabel {
                text-decoration: none;
            }
        """)
        self.top_bar.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Fixed)
        self.top_bar_layout = QHBoxLayout(self.top_bar)
        self.top_bar_layout.setContentsMargins(12, 8, 12, 8)
        self.top_bar_layout.setSpacing(20)

        def _lbl(text, color="#cccccc", bold=False, size=12):
            lbl = QLabel(text)
            lbl.setStyleSheet(
                f"color: {color}; font-size: {size}px; text-decoration: none;"
                + (" font-weight: bold;" if bold else "")
            )
            return lbl

        # Tester side (left)
        self._tester_title_lbl = _lbl("Tester:", "#ffffff", bold=True, size=13)
        self._tester_name_lbl = _lbl("", "#4fc3f7", bold=True, size=13)
        self._tester_detail_lbl = _lbl("", "#888888", size=12)
        tester_layout = QHBoxLayout()
        tester_layout.setSpacing(6)
        tester_layout.addWidget(self._tester_title_lbl)
        tester_layout.addWidget(self._tester_name_lbl)
        tester_layout.addWidget(self._tester_detail_lbl)
        self.top_bar_layout.addLayout(tester_layout)

        self.top_bar_layout.addStretch()

        # Adapter side (right)
        self._adapter_title_lbl = _lbl("Adapter:", "#ffffff", bold=True, size=13)
        self._adapter_name_lbl = _lbl("", "#4fc3f7", bold=True, size=13)
        self._adapter_detail_lbl = _lbl("", "#888888", size=12)
        adapter_layout = QHBoxLayout()
        adapter_layout.setSpacing(6)
        adapter_layout.addWidget(self._adapter_title_lbl)
        adapter_layout.addWidget(self._adapter_name_lbl)
        adapter_layout.addWidget(self._adapter_detail_lbl)
        self.top_bar_layout.addLayout(adapter_layout)

        self._table_visible = True
        self._toggle_table_btn = QPushButton("Hide Table")
        self._toggle_table_btn.setFixedWidth(90)
        self._toggle_table_btn.setStyleSheet(
            "QPushButton { color: #cccccc; background-color: #2e2e45; border: 1px solid #44445a;"
            " border-radius: 4px; padding: 2px 8px; font-size: 12px; }"
            " QPushButton:hover { background-color: #3a3a58; }"
        )
        self._toggle_table_btn.clicked.connect(self._toggle_table)
        self.top_bar_layout.addWidget(self._toggle_table_btn)

        layout.addWidget(self.top_bar)

        # Thin separator line below the top bar
        sep = QFrame()
        sep.setFrameShape(QFrame.Shape.HLine)
        sep.setStyleSheet("background-color: #33334d;")
        sep.setFixedHeight(1)
        layout.addWidget(sep)

        self.die_map = DieMapWidget(shape=self.shape, results=self.results_by_die_pad, result_text="", parent=central)
        self.table = build_results_table(self.shape, self.results_by_die_pad)

        self.splitter = QSplitter(Qt.Orientation.Horizontal)
        self.splitter.addWidget(self.die_map)
        self.splitter.addWidget(self.table)
        self.splitter.setStretchFactor(0, 3)
        self.splitter.setStretchFactor(1, 2)
        self.splitter.setCollapsible(0, False)
        self.splitter.setCollapsible(1, False)
        layout.addWidget(self.splitter)

        self.status_bar = QStatusBar()
        self.setStatusBar(self.status_bar)
        self.status_bar.showMessage(f"Connecting to {port}...")

        self.resize(1200, 800)

        self.reader = SerialReader(port, baud)
        self.reader.line_received.connect(self._on_line)
        self.reader.connection_lost.connect(self._on_disconnect)
        self.reader.hello_received.connect(self._on_hello)
        self.reader.adapter_received.connect(self._on_adapter)
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
            if "DUT_INSERTED" in line:
                self.reader.request_adapter()
            elif "ADAPTER_REMOVED" in line:
                self._adapter_padmap_id = ""
                self._adapter_hw_name = ""
                self._adapter_uid = ""
                self._adapter_ins = 0
                self._adapter_tests = 0
                self._adapter_eol = False
                self._adapter_supported = []
                self._adapter_supported_ids = []
                self._adapter_dut_present = False
                self._update_top_bar()
        elif line.startswith("__ERROR__ "):
            self.status_bar.showMessage(f"Error: {line[10:]}")
        elif line.startswith("ERROR "):
            self.status_bar.showMessage(line)

    def _padmap_display(self) -> str:
        if self._adapter_supported:
            return self._adapter_supported[0]
        if self.shape:
            return f"{self.shape.name} (default)"
        return "unknown"

    def _short_uid(self, uid: str) -> str:
        """Show last 6 hex digits of UID, or full string if shorter."""
        return uid[-6:] if len(uid) >= 6 else uid

    def _update_top_bar(self):
        # Tester side
        if self._tester_name:
            self._tester_name_lbl.setText(self._tester_name)
            details = []
            if self._tester_build:
                details.append(f"Build: {self._tester_build}")
            if self._tester_uid:
                details.append(f"UID: {self._short_uid(self._tester_uid)}")
            self._tester_detail_lbl.setText("  ·  ".join(details))
        else:
            self._tester_name_lbl.setText("")
            self._tester_detail_lbl.setText("")

        # Adapter side
        if self._adapter_padmap_id or self._adapter_hw_name or self._adapter_uid:
            if self._adapter_eol:
                self._adapter_name_lbl.setStyleSheet(
                    "color: #F44336; font-size: 13px; font-weight: bold; text-decoration: none;"
                )
                name_text = f"{self._padmap_display()}  (EOL)"
            else:
                self._adapter_name_lbl.setStyleSheet(
                    "color: #4fc3f7; font-size: 13px; font-weight: bold; text-decoration: none;"
                )
                name_text = self._padmap_display()
            self._adapter_name_lbl.setText(name_text)

            details = []
            if self._adapter_hw_name:
                details.append(self._adapter_hw_name)
            if self._adapter_uid:
                details.append(f"UID: {self._short_uid(self._adapter_uid)}")
            if self._adapter_ins:
                details.append(f"Insertions: {self._adapter_ins}")
            if self._adapter_tests:
                details.append(f"Runs: {self._adapter_tests}")
            self._adapter_detail_lbl.setText("  ·  ".join(details))
        else:
            self._adapter_name_lbl.setText("")
            self._adapter_detail_lbl.setText("")

    def _on_hello(self, kv_str):
        parts = {}
        for kv in kv_str.split():
            k, _, v = kv.partition('=')
            if v:
                parts[k] = v
        self._tester_name = parts.get('name', '')
        self._tester_build = parts.get('build', '')
        self._tester_uid = parts.get('uid', '')
        self._update_top_bar()
        self.reader.request_adapter()

    def _set_shape_from_padmap(self, padmap_id: str):
        new_shape = PADMAP_SHAPES.get(int(padmap_id)) if padmap_id else None
        if new_shape and new_shape != self.shape:
            self.shape = new_shape
            self.results_by_die_pad = {}
            self.pads = []
            self.slots = []
            self.slot_index = {}
            self.outcome = None
            self.good_count = 0
            self.tested_count = 0
            self._rebuild_layout()
            self.setWindowTitle(f"BondTest72 — {self.shape.name} — Live Viewer")

    def _rebuild_layout(self):
        """Recreate die map and table with the current shape."""
        new_die_map = DieMapWidget(
            shape=self.shape, results=self.results_by_die_pad, result_text="", parent=self._central
        )
        new_table = build_results_table(self.shape, self.results_by_die_pad)
        new_table.setVisible(self._table_visible)

        if self.splitter.count() >= 2:
            old_map = self.splitter.replaceWidget(0, new_die_map)
            if old_map:
                old_map.deleteLater()
            old_table = self.splitter.replaceWidget(1, new_table)
            if old_table:
                old_table.deleteLater()
        else:
            while self.splitter.count() > 0:
                w = self.splitter.widget(0)
                self.splitter.replaceWidget(0, QWidget())
                w.deleteLater()
            self.splitter.addWidget(new_die_map)
            self.splitter.addWidget(new_table)

        self.splitter.setStretchFactor(0, 3)
        self.splitter.setStretchFactor(1, 2)
        self.splitter.setCollapsible(0, False)
        self.splitter.setCollapsible(1, False)
        self.die_map = new_die_map
        self.table = new_table

    def _on_adapter(self, raw_line):
        kv_str = raw_line
        if kv_str.startswith("ADAPTER "):
            kv_str = kv_str[8:]
        parts = {}
        for kv in kv_str.split():
            k, _, v = kv.partition('=')
            if v:
                parts[k] = v

        supported_ids = [s for s in parts.get('pm', '').split(',') if s]
        hw_raw = parts.get('hw', '')
        self._adapter_padmap_id = supported_ids[0] if supported_ids else ''
        self._adapter_hw_name = ADAPTER_HW_NAMES.get(hw_raw, f"hw {hw_raw}")
        self._adapter_uid = parts.get('uid', '') or ''
        self._adapter_ins = int(parts.get('ins', '0') or '0')
        self._adapter_tests = int(parts.get('tests', '0') or '0')
        self._adapter_eol = (parts.get('eol', '0') or '0') == '1'
        self._adapter_supported = [PADMAP_NAMES.get(pm, pm) for pm in supported_ids]
        self._adapter_supported_ids = supported_ids
        self._adapter_dut_present = (parts.get('dut', '0') or '0') == '1'
        self._update_top_bar()
        self._set_shape_from_padmap(self._adapter_padmap_id)

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
            self.reader.request_adapter()

    def _refresh_table(self):
        new_table = build_results_table(self.shape, self.results_by_die_pad)
        new_table.setVisible(self._table_visible)
        if self.splitter.count() >= 2:
            old = self.splitter.replaceWidget(1, new_table)
            if old:
                old.deleteLater()
        else:
            self.splitter.addWidget(new_table)
        self.splitter.setStretchFactor(0, 3)
        self.splitter.setStretchFactor(1, 2)
        self.splitter.setCollapsible(1, False)
        self.table = new_table

    def _toggle_table(self):
        self._table_visible = not self._table_visible
        self.table.setVisible(self._table_visible)
        self._toggle_table_btn.setText("Hide Table" if self._table_visible else "Show Table")

    def _on_disconnect(self):
        self.status_bar.showMessage("Connection lost")
        self._tester_name = ""
        self._tester_build = ""
        self._tester_uid = ""
        self._adapter_padmap_id = ""
        self._adapter_hw_name = ""
        self._adapter_uid = ""
        self._adapter_ins = 0
        self._adapter_tests = 0
        self._adapter_eol = False
        self._adapter_supported = []
        self._adapter_supported_ids = []
        self._adapter_dut_present = False
        self._update_top_bar()

    def closeEvent(self, event):
        self.reader.stop()
        super().closeEvent(event)


def main():
    parser = argparse.ArgumentParser(description="BondTest72 live serial viewer")
    parser.add_argument("--port", required=True, help="Serial port (e.g. /dev/tty.usbmodem1101 or COM3)")
    parser.add_argument("--baud", type=int, default=115200, help="Baud rate (default: 115200)")
    parser.add_argument("--padmap", type=int, default=None, help="Pad map ID (default: auto-detect from adapter)")
    args = parser.parse_args()

    shape = PADMAP_SHAPES.get(args.padmap) if args.padmap is not None else None

    app = QApplication(sys.argv)
    app.setStyle("Fusion")
    app.styleHints().setColorScheme(Qt.ColorScheme.Dark)
    window = LiveViewer(shape, args.port, args.baud)
    window.show()
    ret = app.exec()
    window.reader.stop()
    sys.exit(ret)


if __name__ == "__main__":
    main()
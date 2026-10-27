"""Main window: connect to the tester, pick a pad map, run tests, view and record results."""

from __future__ import annotations

import dataclasses
from pathlib import Path

import serial
from PySide6.QtCore import QEvent, QRectF, QSettings, Qt, QTimer
from PySide6.QtGui import QAction, QFont, QIcon, QKeySequence, QPainter, QPalette, QPen, QPixmap
from PySide6.QtWidgets import (
    QComboBox, QDockWidget, QFileDialog, QFrame, QHBoxLayout, QLabel, QLineEdit,
    QListWidget, QListWidgetItem, QMainWindow, QMessageBox, QPlainTextEdit, QPushButton,
    QSplitter, QTabWidget, QToolBar, QToolButton, QVBoxLayout, QWidget,
)

from . import __version__, protocol as p
from . import results_jsonl
from .connection import TesterConnection
from .die_shapes import shape_for_padmap
from .die_view import GOOD_COLOR, OPEN_COLOR, DieLegend, DieView
from .models import TestResult
from .pad_table import PadGroup
from .results_panel import PadGroupPanel
from .session import Update
from .tester_serial import find_tester_ports

PORT_POLL_MS = 1500          # how often to look for the tester while disconnected
LOG_MAX_LINES = 5000
DEFAULT_PADMAP = 2            # die drawn while no tester or adapter reports one
TOOLBAR_CONTROL_HEIGHT = 30   # content height; borders add ~2 px
RUN_BLUE = "#2a78d6"          # primary action; white text reads on it in both themes
RUN_BLUE_HOVER = "#3987e5"
RUN_BLUE_PRESSED = "#1f62b3"


def default_results_folder() -> Path:
    """results/ under the folder the app was started from."""
    return Path.cwd() / "results"


class _PanelToggle(QToolButton):
    """Shows or hides the bottom panel. Its icon is a window whose bottom strip
    is filled while the panel is shown, drawn in the theme's text colour."""

    def __init__(self, **kwargs) -> None:
        super().__init__(**kwargs)
        self.setCheckable(True)

    def setDefaultAction(self, action: QAction) -> None:
        super().setDefaultAction(action)
        self._update_icon()

    def changeEvent(self, event: QEvent) -> None:
        if event.type() == QEvent.Type.PaletteChange and self.defaultAction():   # light/dark switch
            self._update_icon()
        super().changeEvent(event)

    def _update_icon(self) -> None:
        icon = QIcon()
        for state, filled in ((QIcon.State.On, True), (QIcon.State.Off, False)):
            icon.addPixmap(self._pixmap(filled), QIcon.Mode.Normal, state)
        # On the action: the button takes its icon from its default action.
        self.defaultAction().setIcon(icon)

    def _pixmap(self, filled: bool) -> QPixmap:
        size, ratio = 16, self.devicePixelRatioF()
        pixmap = QPixmap(round(size * ratio), round(size * ratio))
        pixmap.setDevicePixelRatio(ratio)
        pixmap.fill(Qt.GlobalColor.transparent)
        color = self.palette().color(QPalette.ColorRole.ButtonText)
        painter = QPainter(pixmap)
        painter.setRenderHint(QPainter.RenderHint.Antialiasing)
        painter.setPen(QPen(color, 1.2))
        window = QRectF(1.5, 2.5, 13, 11)
        painter.drawRoundedRect(window, 1.5, 1.5)
        strip = QRectF(window.left(), 9.5, window.width(), window.bottom() - 9.5)
        if filled:
            painter.setBrush(color)
            painter.drawRoundedRect(strip, 1.5, 1.5)
        else:
            painter.drawLine(strip.topLeft(), strip.topRight())
        painter.end()
        return pixmap


class MainWindow(QMainWindow):
    def __init__(self) -> None:
        super().__init__()
        self.setWindowTitle("BondTest72")
        self._settings = QSettings()
        self._conn = TesterConnection(self)
        self._shown: TestResult | None = None      # result on screen; None = live view of the tester
        self._padmap: int | None = None            # last active pad map the tester reported
        self._history: list[TestResult] = []
        self._user_disconnected = False            # suppresses auto-connect until Connect is pressed

        self._build_ui()
        self._wire()
        self._refresh_ports()
        self._update_controls()
        self._show_result(None)

        self._port_timer = QTimer(self, interval=PORT_POLL_MS, timeout=self._poll_ports)
        self._port_timer.start()
        QTimer.singleShot(0, self._poll_ports)

        geometry = self._settings.value("window/geometry")
        if geometry is not None:
            self.restoreGeometry(geometry)
        else:
            self.resize(1100, 760)

    # ——— construction ———————————————————————————————————————————————————————

    def _build_ui(self) -> None:
        # Toolbar: connection, pad map, sample ID, run.
        bar = QToolBar("Tester", self, movable=False)
        self.addToolBar(bar)
        self._port_combo = QComboBox(minimumContentsLength=18)
        self._port_combo.setSizeAdjustPolicy(QComboBox.SizeAdjustPolicy.AdjustToContents)
        self._connect_button = QPushButton("Connect")
        self._padmap_combo = QComboBox(minimumContentsLength=22)
        self._padmap_combo.setSizeAdjustPolicy(QComboBox.SizeAdjustPolicy.AdjustToContents)
        self._default_button = QPushButton("Set as default")
        self._default_button.setToolTip("Store this pad map in the adapter EEPROM as the one "
                                        "selected whenever the adapter is detected")
        self._sample_edit = QLineEdit(placeholderText="Sample ID (optional)", clearButtonEnabled=True)
        self._sample_edit.setFixedWidth(220)
        self._run_button = QPushButton("Run test", objectName="runButton")
        run_font = QFont(self._run_button.font())
        run_font.setBold(True)
        self._run_button.setFont(run_font)
        self._run_button.setMinimumWidth(110)
        # Filled in once the bottom dock (whose show/hide action it uses) exists.
        self._panel_button = _PanelToggle(objectName="panelButton")

        # The controls sit in one container with its own margins: QToolBar
        # ignores padding from a stylesheet, and the theme's stylesheet would
        # override plain setMinimumHeight() on combo boxes and line edits, so
        # the control height is set by a stylesheet scoped to this bar.
        controls = QWidget()
        row = QHBoxLayout(controls)
        row.setContentsMargins(8, 3, 8, 3)     # 32 px controls + toolbar frame → 42 px bar
        row.setSpacing(8)
        row.addWidget(QLabel("Port"))
        row.addWidget(self._port_combo)
        row.addWidget(self._connect_button)
        row.addWidget(self._separator())
        row.addWidget(QLabel("Pad map"))
        row.addWidget(self._padmap_combo)
        row.addWidget(self._default_button)
        row.addWidget(self._separator())
        row.addWidget(self._sample_edit)
        row.addStretch(1)
        row.addWidget(self._panel_button)
        row.addWidget(self._run_button)
        # Run test is the primary action: a filled blue button, plain when disabled.
        controls.setStyleSheet(
            "QComboBox, QLineEdit, QPushButton {"
            f" min-height: {TOOLBAR_CONTROL_HEIGHT}px; max-height: {TOOLBAR_CONTROL_HEIGHT}px;"
            " padding-top: 0; padding-bottom: 0; }"
            # A square button as tall as the bordered controls beside it.
            f"QToolButton#panelButton {{ min-width: {TOOLBAR_CONTROL_HEIGHT + 2}px;"
            f" max-width: {TOOLBAR_CONTROL_HEIGHT + 2}px; min-height: {TOOLBAR_CONTROL_HEIGHT + 2}px;"
            f" max-height: {TOOLBAR_CONTROL_HEIGHT + 2}px; padding: 0; }}"
            f"QPushButton#runButton {{ background: {RUN_BLUE}; border: 1px solid {RUN_BLUE};"
            " color: white; border-radius: 4px; padding-left: 18px; padding-right: 18px; }"
            f"QPushButton#runButton:hover {{ background: {RUN_BLUE_HOVER}; border-color: {RUN_BLUE_HOVER}; }}"
            f"QPushButton#runButton:pressed {{ background: {RUN_BLUE_PRESSED}; border-color: {RUN_BLUE_PRESSED}; }}"
            "QPushButton#runButton:disabled { background: transparent;"
            " border: 1px solid rgba(128, 128, 128, 0.35); color: rgba(128, 128, 128, 0.8); }")
        bar.addWidget(controls)

        # Centre: outcome banner and die drawing on the left, pad table on the right.
        self._banner = QLabel(alignment=Qt.AlignmentFlag.AlignCenter)
        banner_font = QFont(self._banner.font())
        banner_font.setPointSizeF(banner_font.pointSizeF() * 1.8)
        banner_font.setBold(True)
        self._banner.setFont(banner_font)
        self._banner_detail = QLabel(alignment=Qt.AlignmentFlag.AlignCenter)
        self._die_view = DieView()

        left = QWidget()
        left_layout = QVBoxLayout(left)
        left_layout.setContentsMargins(8, 8, 8, 8)
        left_layout.addWidget(self._banner)
        left_layout.addWidget(self._banner_detail)
        left_layout.addWidget(self._die_view, 1)
        left_layout.addWidget(DieLegend())

        # Right: one tab per pad group, each a table with its graph underneath.
        self._panels = [PadGroupPanel(PadGroup.IO), PadGroupPanel(PadGroup.POWER)]
        self._group_tabs = QTabWidget(documentMode=True)   # left-aligned, like the bottom tabs
        for panel in self._panels:
            self._group_tabs.addTab(panel, panel.group.value)

        splitter = QSplitter(Qt.Orientation.Horizontal)
        splitter.addWidget(left)
        splitter.addWidget(self._group_tabs)
        splitter.setStretchFactor(0, 1)
        splitter.setStretchFactor(1, 1)
        splitter.setSizes([520, 680])
        self.setCentralWidget(splitter)

        # Bottom dock: results history and the raw serial log.
        self._history_label = QLabel()
        self._history_list = QListWidget()
        history = QWidget()
        history_layout = QVBoxLayout(history)
        history_layout.setContentsMargins(4, 4, 4, 4)
        history_layout.addWidget(self._history_label)
        history_layout.addWidget(self._history_list)

        self._log = QPlainTextEdit(readOnly=True)
        self._log.setMaximumBlockCount(LOG_MAX_LINES)
        self._log.setFont(QFont("Menlo, Consolas, monospace"))

        tabs = QTabWidget(documentMode=True)   # plain tabs, no frame for them to straddle
        tabs.addTab(history, "History")
        tabs.addTab(self._log, "Serial log")
        dock = QDockWidget("Results", self, objectName="results_dock")
        dock.setFeatures(QDockWidget.DockWidgetFeature.NoDockWidgetFeatures)
        dock.setTitleBarWidget(QWidget())          # the tabs name the contents; no title row
        dock.setWidget(tabs)
        self.addDockWidget(Qt.DockWidgetArea.BottomDockWidgetArea, dock)
        self.resizeDocks([dock], [170], Qt.Orientation.Vertical)
        # Shown or hidden from the toolbar, the View menu or Ctrl+J. (Not the dock's
        # own toggleViewAction: Qt disables it for a dock that can't be closed.)
        self._dock = dock
        self._panel_action = QAction("&History and serial log", self, checkable=True,
                                     shortcut=QKeySequence("Ctrl+J"),
                                     toolTip="Show or hide the history and serial log (Ctrl+J)")
        self._panel_action.toggled.connect(dock.setVisible)
        self._panel_action.setChecked(self._settings.value("window/bottom_panel", True, type=bool))
        dock.setVisible(self._panel_action.isChecked())
        self._panel_button.setDefaultAction(self._panel_action)

        # Status bar.
        self._state_label = QLabel()
        self._tester_label = QLabel()
        self._adapter_label = QLabel()
        status = self.statusBar()
        status.addPermanentWidget(self._adapter_label)
        status.addPermanentWidget(self._tester_label)
        status.addPermanentWidget(self._state_label)

        # Menus.
        file_menu = self.menuBar().addMenu("&File")
        self._open_action = QAction("&Open results file…", self, shortcut=QKeySequence.StandardKey.Open)
        self._folder_action = QAction("Results &folder…", self)
        self._record_action = QAction("&Record results", self, checkable=True)
        self._record_action.setChecked(self._settings.value("record", True, type=bool))
        quit_action = QAction("&Quit", self, shortcut=QKeySequence.StandardKey.Quit)
        quit_action.triggered.connect(self.close)
        for action in (self._open_action, self._folder_action, self._record_action):
            file_menu.addAction(action)
        file_menu.addSeparator()
        file_menu.addAction(quit_action)

        tester_menu = self.menuBar().addMenu("&Tester")
        self._run_action = QAction("&Run test", self, shortcut=QKeySequence("Ctrl+R"))
        self._last_result_action = QAction("Show &last result", self, shortcut=QKeySequence("Ctrl+L"))
        self._live_action = QAction("Back to &live view", self, shortcut=QKeySequence("Esc"))
        for action in (self._run_action, self._last_result_action, self._live_action):
            tester_menu.addAction(action)
            self.addAction(action)                 # keep shortcuts working without the menu open

        view_menu = self.menuBar().addMenu("&View")
        view_menu.addAction(self._panel_action)
        self.addAction(self._panel_action)

        help_menu = self.menuBar().addMenu("&Help")
        about = QAction("&About", self)
        about.triggered.connect(lambda: QMessageBox.about(
            self, "BondTest72", f"BondTest72 desktop app {__version__}"))
        help_menu.addAction(about)

    @staticmethod
    def _separator() -> QFrame:
        line = QFrame()
        line.setFrameShape(QFrame.Shape.VLine)
        line.setFrameShadow(QFrame.Shadow.Plain)
        line.setEnabled(False)      # drawn in the theme's muted tone
        return line

    def _wire(self) -> None:
        self._connect_button.clicked.connect(self._toggle_connection)
        self._padmap_combo.activated.connect(self._padmap_chosen)
        self._default_button.clicked.connect(self._set_default_padmap)
        self._run_button.clicked.connect(self._run)
        self._run_action.triggered.connect(self._run)
        self._last_result_action.triggered.connect(self._conn.get_results)
        self._live_action.triggered.connect(lambda: self._show_result(None))
        self._open_action.triggered.connect(self._open_results_file)
        self._folder_action.triggered.connect(self._choose_results_folder)
        self._record_action.toggled.connect(lambda on: self._settings.setValue("record", on))

        self._die_view.pad_clicked.connect(self._select_pad_row)
        for panel in self._panels:
            panel.pad_selected.connect(self._pad_selected_in_panel)
        self._history_list.currentRowChanged.connect(self._history_selected)

        self._conn.connected.connect(self._on_connected)
        self._conn.disconnected.connect(self._on_disconnected)
        self._conn.line_received.connect(lambda line: self._log.appendPlainText(line))
        self._conn.line_sent.connect(lambda line: self._log.appendPlainText(f"→ {line}"))
        self._conn.updated.connect(self._on_update)
        self._conn.result_ready.connect(self._on_result)

    # ——— connection —————————————————————————————————————————————————————————

    def _refresh_ports(self) -> list[str]:
        ports = find_tester_ports()
        devices = [port.device for port in ports]
        current = self._port_combo.currentData()
        if devices != [self._port_combo.itemData(i) for i in range(self._port_combo.count())]:
            self._port_combo.clear()
            for port in ports:
                self._port_combo.addItem(port.device, port.device)
            if current in devices:
                self._port_combo.setCurrentIndex(devices.index(current))
        if not devices:
            self._port_combo.setPlaceholderText("No tester found")
        return devices

    def _poll_ports(self) -> None:
        if self._conn.is_open:
            return
        devices = self._refresh_ports()
        if len(devices) == 1 and not self._user_disconnected:
            self._open_port(devices[0], quiet=True)
        self._update_controls()

    def _toggle_connection(self) -> None:
        if self._conn.is_open:
            self._user_disconnected = True
            self._conn.close()
            return
        self._user_disconnected = False
        port = self._port_combo.currentData()
        if port:
            self._open_port(port, quiet=False)

    def _open_port(self, port: str, quiet: bool) -> None:
        try:
            self._conn.open(port)
        except (serial.SerialException, OSError) as exc:
            if quiet:
                self.statusBar().showMessage(f"Can't open {port}: {exc}", 5000)
            else:
                QMessageBox.warning(self, "BondTest72", f"Can't open {port}:\n{exc}")

    def _on_connected(self, port: str) -> None:
        self.statusBar().showMessage(f"Connected to {port}", 3000)
        self._update_controls()

    def _on_disconnected(self, reason: str) -> None:
        message = f"Disconnected: {reason}" if reason else "Disconnected"
        self.statusBar().showMessage(message, 5000)
        self._log.appendPlainText(f"— {message}")
        self._update_controls()
        if self._shown is None:
            self._show_result(None)

    # ——— tester updates —————————————————————————————————————————————————————

    def _on_update(self, update: Update) -> None:
        if isinstance(update.message, p.TestStart):
            self._show_result(None)
        elif isinstance(update.message, p.Error):
            self.statusBar().showMessage(f"Tester error {update.message.code}: {update.message.msg}", 5000)
        self._update_controls()
        # A different pad map makes the result on screen stale: go back to the
        # live view, which draws the new map's die. (Not on a reconnect or a
        # re-inserted adapter with the same map, so an opened result stays.)
        padmap = self._conn.session.active_padmap
        if padmap is not None and padmap != self._padmap:
            if self._padmap is not None:
                self._shown = None
            self._padmap = padmap
        if self._shown is None:
            self._show_result(None)                # live view follows the active pad map and state

    def _on_result(self, result: TestResult, live: bool) -> None:
        sample = self._sample_edit.text().strip()
        if live and sample:
            result = dataclasses.replace(result, panel_id=sample)
        if live and self._record_action.isChecked():
            self._record(result)
        self._add_to_history(result, select=True)

    def _record(self, result: TestResult) -> None:
        path = results_jsonl.daily_file(self._results_folder(), result.ts.date())
        try:
            results_jsonl.append(path, result)
        except OSError as exc:
            QMessageBox.warning(self, "BondTest72", f"Couldn't save the result to {path}:\n{exc}")
            return
        self.statusBar().showMessage(f"Saved to {path}", 4000)

    # ——— commands ———————————————————————————————————————————————————————————

    def _run(self) -> None:
        if self._conn.session.can_run:
            self._conn.run()

    def _padmap_chosen(self, index: int) -> None:
        padmap_id = self._padmap_combo.itemData(index)
        if padmap_id is not None and padmap_id != self._conn.session.active_padmap:
            self._conn.set_padmap(padmap_id)

    def _set_default_padmap(self) -> None:
        active = self._conn.session.active_padmap
        if active is not None and self._conn.session.can_set_padmap:
            self._conn.set_default_padmap(active)

    # ——— display ————————————————————————————————————————————————————————————

    def _update_controls(self) -> None:
        session = self._conn.session
        connected = self._conn.is_open
        self._connect_button.setText("Disconnect" if connected else "Connect")
        self._connect_button.setEnabled(connected or self._port_combo.count() > 0)
        self._port_combo.setEnabled(not connected)
        self._run_button.setEnabled(connected and session.can_run)
        self._run_action.setEnabled(connected and session.can_run)
        self._last_result_action.setEnabled(connected and session.state in ("ADAPTER_DETECTED", "READY", "PASS", "FAIL"))

        # Pad map choices, rebuilt only when the list, the default or the active map changes.
        entries = [(m.id, f"{m.id} · {m.name}" + ("  (default)" if m.default else ""))
                   for m in session.padmaps]
        shown = [(self._padmap_combo.itemData(i), self._padmap_combo.itemText(i))
                 for i in range(self._padmap_combo.count())]
        if entries != shown:
            self._padmap_combo.clear()
            for padmap_id, text in entries:
                self._padmap_combo.addItem(text, padmap_id)
        active = session.active_padmap
        index = self._padmap_combo.findData(active) if active is not None else -1
        if index != self._padmap_combo.currentIndex():
            self._padmap_combo.setCurrentIndex(index)
        self._padmap_combo.setEnabled(connected and session.can_set_padmap)
        self._padmap_combo.setPlaceholderText("—")
        active_is_default = any(m.id == active and m.default for m in session.padmaps)
        self._default_button.setEnabled(connected and session.can_set_padmap
                                        and active is not None and not active_is_default)

        state = session.state or ("—" if connected else "Not connected")
        self._state_label.setText(f"  {state.replace('_', ' ').title()}  ")
        hello = session.hello
        self._tester_label.setText(f"Tester {hello.uid[-6:]} · fw {hello.build}" if hello else "")
        adapter = session.adapter
        self._adapter_label.setText(
            f"Adapter {adapter.aid[-6:]} · {adapter.ins} insertions · {adapter.tests} tests" if adapter else "")

    def _show_result(self, result: TestResult | None) -> None:
        """Show a result, or the live tester view (active pad map, no result) for None."""
        self._shown = result
        for i, panel in enumerate(self._panels):
            panel.set_result(result)
            count = f" ({panel.pad_count})" if result else ""
            self._group_tabs.setTabText(i, panel.group.value + count)
            self._group_tabs.setTabEnabled(i, result is None or panel.pad_count > 0)
        self._die_view.set_result(result)
        self._die_view.set_selected(None)

        if result is None:
            session = self._conn.session
            padmap = session.active_padmap
            # With no tester or adapter, show the usual die rather than an empty view.
            shown_padmap = padmap if padmap is not None else DEFAULT_PADMAP
            self._die_view.set_shape(shape_for_padmap(shown_padmap),
                                     f"No die drawing for pad map {shown_padmap}")
            testing = session.state == "TESTING"
            self._set_banner("Testing…" if testing else "Ready" if session.can_run else "", None)
            self._banner_detail.setText(f"{padmap} · {session.padmap_name(padmap)}" if padmap else "")
            if not self._history_list.signalsBlocked():
                self._history_list.blockSignals(True)
                self._history_list.setCurrentRow(-1)
                self._history_list.blockSignals(False)
            return

        ctx = result.context
        pm = ctx.pm if ctx else None
        self._die_view.set_shape(shape_for_padmap(pm) if pm else None,
                                 f"No die drawing for pad map {pm}" if pm else "Unknown pad map")
        verdict = f"{result.outcome}  {result.good}/{result.tested}"
        if result.reason:
            verdict += f"  · {result.reason.replace('_', ' ').lower()}"
        self._set_banner(verdict, result.passed)
        details = [result.ts.strftime("%Y-%m-%d %H:%M:%S")]
        if ctx:
            details.append(f"pad map {ctx.pm}" + (f" · {ctx.pm_name}" if ctx.pm_name else ""))
        if result.panel_id:
            details.append(result.panel_id)
        self._banner_detail.setText("   ".join(details))

    def _set_banner(self, text: str, passed: bool | None) -> None:
        self._banner.setText(text)
        color = "" if passed is None else (GOOD_COLOR if passed else OPEN_COLOR).name()
        self._banner.setStyleSheet(f"color: {color};" if color else "")

    def _select_pad_row(self, die_pad: int) -> None:
        """A pad was clicked on the die: show it in whichever group has it."""
        for i, panel in enumerate(self._panels):
            if panel.select_pad(die_pad):
                self._group_tabs.setCurrentIndex(i)

    def _pad_selected_in_panel(self, die_pad: int) -> None:
        self._die_view.set_selected(die_pad)
        for panel in self._panels:
            if panel is not self.sender():
                panel.select_pad(None)

    # ——— history and files ——————————————————————————————————————————————————

    def _add_to_history(self, result: TestResult, select: bool) -> None:
        self._history.append(result)
        item = QListWidgetItem(self._history_text(result))
        item.setForeground(GOOD_COLOR if result.passed else OPEN_COLOR)
        self._history_list.addItem(item)
        self._history_label.setText(f"{len(self._history)} results")
        if select:
            self._history_list.setCurrentRow(len(self._history) - 1)

    @staticmethod
    def _history_text(result: TestResult) -> str:
        parts = [result.ts.strftime("%m-%d %H:%M:%S"), f"{result.outcome} {result.good}/{result.tested}"]
        if result.context:
            parts.append(f"pm {result.context.pm}")
        if result.panel_id:
            parts.append(result.panel_id)
        return "   ".join(parts)

    def _history_selected(self, row: int) -> None:
        if 0 <= row < len(self._history):
            self._show_result(self._history[row])

    def _open_results_file(self) -> None:
        start = str(self._results_folder())
        name, _ = QFileDialog.getOpenFileName(self, "Open results file", start, "Results (*.jsonl);;All files (*)")
        if not name:
            return
        try:
            results = results_jsonl.load(Path(name))
        except OSError as exc:
            QMessageBox.warning(self, "BondTest72", f"Can't read {name}:\n{exc}")
            return
        self._history.clear()
        self._history_list.clear()
        for result in results:
            self._add_to_history(result, select=False)
        self._history_label.setText(f"{len(results)} results from {Path(name).name}")
        if results:
            self._history_list.setCurrentRow(len(results) - 1)

    def _results_folder(self) -> Path:
        return Path(self._settings.value("results_folder", str(default_results_folder())))

    def _choose_results_folder(self) -> None:
        folder = QFileDialog.getExistingDirectory(self, "Results folder", str(self._results_folder()))
        if folder:
            self._settings.setValue("results_folder", folder)
            self.statusBar().showMessage(f"Results are saved in {folder}", 4000)

    # ——— shutdown ———————————————————————————————————————————————————————————

    def closeEvent(self, event) -> None:
        self._settings.setValue("window/geometry", self.saveGeometry())
        self._settings.setValue("window/bottom_panel", self._panel_action.isChecked())
        self._port_timer.stop()
        self._conn.close()
        super().closeEvent(event)

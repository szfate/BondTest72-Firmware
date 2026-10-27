"""Qt side of the tester connection.

TesterConnection owns the serial port (tester_serial.TesterSerial) and the
TesterSession, and moves received lines from the reader thread onto the Qt
event loop, so everything the window sees arrives as ordinary signals.
"""

from __future__ import annotations

from PySide6.QtCore import QObject, Signal, Slot

from . import protocol as p
from .models import TestResult
from .session import TesterSession, Update
from .tester_serial import TesterSerial


class TesterConnection(QObject):
    line_received = Signal(str)              # every line, for the log view
    line_sent = Signal(str)
    updated = Signal(object)                 # session.Update, after the session has applied it
    result_ready = Signal(object, bool)      # TestResult, live (False = GET_RESULTS replay)
    connected = Signal(str)                  # port
    disconnected = Signal(str)               # reason; empty for a normal close

    # Reader-thread → event-loop hops. Emitting a signal from another thread
    # queues the call into this object's thread.
    _line_from_thread = Signal(str)
    _closed_from_thread = Signal(str)

    def __init__(self, parent: QObject | None = None) -> None:
        super().__init__(parent)
        self.session = TesterSession()
        self._serial: TesterSerial | None = None
        self._line_from_thread.connect(self._on_line)
        self._closed_from_thread.connect(self._on_closed)

    @property
    def is_open(self) -> bool:
        return self._serial is not None

    @property
    def port(self) -> str:
        return self._serial.port if self._serial else ""

    def open(self, port: str) -> None:
        """Open `port` and query the tester. Raises serial.SerialException on failure."""
        self.close()
        self.session.reset()
        self._serial = TesterSerial(
            port,
            on_line=self._line_from_thread.emit,
            on_closed=lambda exc: self._closed_from_thread.emit(str(exc) if exc else ""),
        )
        self.connected.emit(port)
        for command in self.session.startup_commands():
            self.send(command)

    def close(self) -> None:
        if self._serial is not None:
            self._serial.close()        # reports through _on_closed

    def send(self, command: str) -> None:
        if self._serial is None:
            return
        try:
            self._serial.send(command)
        except OSError as exc:          # SerialException is an OSError
            self._closed_from_thread.emit(str(exc))
            return
        self.line_sent.emit(command.rstrip("\n"))

    # ——— commands ———————————————————————————————————————————————————————————

    def run(self) -> None:
        self.send(p.cmd_run())

    def set_padmap(self, padmap_id: int) -> None:
        self.send(p.cmd_set_padmap(padmap_id))

    def set_default_padmap(self, padmap_id: int) -> None:
        self.send(p.cmd_set_default_padmap(padmap_id))

    def get_results(self) -> None:
        self.send(p.cmd_get_results())

    # ——— internals ——————————————————————————————————————————————————————————

    @Slot(str)
    def _on_line(self, line: str) -> None:
        self.line_received.emit(line)
        update: Update = self.session.feed(line)
        for command in update.send:
            self.send(command)
        self.updated.emit(update)
        if update.result is not None:
            result: TestResult = update.result
            self.result_ready.emit(result, update.live)

    @Slot(str)
    def _on_closed(self, reason: str) -> None:
        if self._serial is None:
            return
        serial, self._serial = self._serial, None
        serial.close()
        self.session.reset()
        self.disconnected.emit(reason)

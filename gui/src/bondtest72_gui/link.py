"""Serial connection to the tester: port discovery and a background line reader.

Kept free of any GUI toolkit so it can be used from scripts and tests; the GUI
wraps it and forwards lines to its own event loop.
"""

from __future__ import annotations

import threading
from collections.abc import Callable
from dataclasses import dataclass

import serial
from serial.tools import list_ports

BAUD_RATE = 115200

# USB vendor ID of the Raspberry Pi RP2350 (the tester's Pico 2). Any Pico-based
# serial device matches, so the GUI confirms the device with HELLO afterwards.
RPI_USB_VID = 0x2E8A


@dataclass(frozen=True)
class PortInfo:
    device: str          # e.g. /dev/cu.usbmodem11301 or COM3
    description: str
    serial_number: str


def find_tester_ports() -> list[PortInfo]:
    """Serial ports that look like a BondTest72 (RP2350 USB serial), sorted by name."""
    ports = [
        PortInfo(p.device, p.description or "", p.serial_number or "")
        for p in list_ports.comports()
        if p.vid == RPI_USB_VID
    ]
    return sorted(ports, key=lambda p: p.device)


class TesterLink:
    """An open connection to the tester.

    Received lines are delivered to `on_line` from a background thread, without
    the trailing newline. When the connection ends — closed by close() or lost,
    e.g. USB unplugged — `on_closed` is called once, with the exception that
    ended it or None for a normal close.

    `port` is a device name or any pyserial URL (tests use "loop://").
    """

    __test__ = False  # not a pytest test class, despite the name

    def __init__(self, port: str,
                 on_line: Callable[[str], None],
                 on_closed: Callable[[Exception | None], None] | None = None,
                 baud: int = BAUD_RATE) -> None:
        self._on_line = on_line
        self._on_closed = on_closed
        # A short timeout keeps readline() responsive to close().
        self._serial = serial.serial_for_url(port, baudrate=baud, timeout=0.1)
        self._write_lock = threading.Lock()
        self._stop = threading.Event()
        self._closed_lock = threading.Lock()     # close() and the reader may both report
        self._closed_reported = False
        self._reader = threading.Thread(target=self._read_loop, name="tester-reader", daemon=True)
        self._reader.start()

    @property
    def port(self) -> str:
        return self._serial.port or ""

    def send(self, line: str) -> None:
        """Write one command line (see the cmd_* helpers in protocol.py)."""
        if not line.endswith("\n"):
            line += "\n"
        with self._write_lock:
            self._serial.write(line.encode("ascii"))

    def close(self) -> None:
        """Stop reading and close the port. Safe to call more than once."""
        self._stop.set()
        if self._reader is not threading.current_thread():
            self._reader.join(timeout=1.0)
        self._serial.close()
        self._report_closed(None)

    def _read_loop(self) -> None:
        buffer = b""
        try:
            while not self._stop.is_set():
                chunk = self._serial.readline()
                if not chunk:
                    continue                      # timeout — check the stop flag
                buffer += chunk
                if not buffer.endswith(b"\n"):
                    continue                      # partial line; keep reading
                text = buffer.decode("utf-8", errors="replace").rstrip("\r\n")
                buffer = b""
                if text:
                    self._on_line(text)
        except (serial.SerialException, OSError) as exc:
            if not self._stop.is_set():
                self._report_closed(exc)

    def _report_closed(self, exc: Exception | None) -> None:
        with self._closed_lock:
            if self._closed_reported:
                return
            self._closed_reported = True
        if self._on_closed:
            self._on_closed(exc)

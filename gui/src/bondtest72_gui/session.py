"""The tester as the host sees it: identity, adapter, pad maps and state.

TesterSession turns the stream of received lines into current state and
decides which queries to send to keep that state fresh. The tester announces
changes with events (EVENT DUT_INSERTED, ...) but not always the state they
lead to, so the session follows each one with GET_ADAPTER, whose reply carries
state=. Kept free of any GUI toolkit, like the rest of the core.
"""

from __future__ import annotations

from collections.abc import Callable
from dataclasses import dataclass, field
from datetime import datetime

from . import protocol as p
from .assembler import ResultAssembler
from .models import TestResult

# States in which RUN and SET_PADMAP are accepted (protocol doc, State Machine).
RUN_STATES = frozenset({"READY", "PASS", "FAIL"})
PADMAP_STATES = frozenset({"ADAPTER_DETECTED", "READY", "WRONG_ORIENTATION", "PASS", "FAIL"})

# ERROR codes that mean there is no usable adapter.
_NO_ADAPTER_ERRORS = frozenset({1, 7})   # NO_ADAPTER, ADAPTER_NOT_PROVISIONED


@dataclass
class Update:
    """What one received line changed."""

    message: p.Message | None
    result: TestResult | None = None   # a finished test result, if this line completed one
    live: bool = False                 # True if `result` is a new test, False for a GET_RESULTS replay
    send: list[str] = field(default_factory=list)   # commands to write to the tester


class TesterSession:
    """Feed it every received line; read the current state from its attributes."""

    __test__ = False  # not a pytest test class, despite the name

    def __init__(self, clock: Callable[[], datetime] = datetime.now) -> None:
        self._clock = clock
        self._assembler = ResultAssembler(clock=clock)
        self.hello: p.Hello | None = None
        self.adapter: p.AdapterInfo | None = None
        self.state: str | None = None             # None until the tester reports one (v1 never does)
        self.padmaps: list[p.PadmapEntry] = []
        self.last_error: p.Error | None = None
        self.fault: str | None = None
        self._padmaps_pending: list[p.PadmapEntry] = []
        self._test_running = False                # between TEST_START and SUMMARY

    # ——— queries ————————————————————————————————————————————————————————————

    @property
    def active_padmap(self) -> int | None:
        if self.adapter and self.adapter.active is not None:
            return self.adapter.active
        return next((m.id for m in self.padmaps if m.active), None)

    @property
    def can_run(self) -> bool:
        if self.state is not None:
            return self.state in RUN_STATES
        return bool(self.adapter and self.adapter.dut)    # v1 firmware: best guess

    @property
    def can_set_padmap(self) -> bool:
        return self.state in PADMAP_STATES and bool(self.padmaps)

    def padmap_name(self, padmap_id: int) -> str:
        return next((m.name for m in self.padmaps if m.id == padmap_id), "")

    # ——— input ——————————————————————————————————————————————————————————————

    def startup_commands(self) -> list[str]:
        """Commands to send right after opening the port."""
        return [p.cmd_hello(), p.cmd_get_adapter(), p.cmd_list_padmaps()]

    def reset(self) -> None:
        """Forget everything, e.g. after the connection closed."""
        self.__init__(clock=self._clock)

    def feed(self, line: str) -> Update:
        msg = p.parse_line(line)
        update = Update(msg)
        update.result = self._assembler.feed(msg)
        if update.result is not None:
            update.live = self._test_running

        match msg:
            case p.Hello():
                self.hello = msg
            case p.AdapterInfo():
                self.adapter = msg
                if msg.state is not None:
                    self.state = msg.state
            case p.AdapterDetected():
                self.state = "ADAPTER_DETECTED"
                update.send += [p.cmd_get_adapter(), p.cmd_list_padmaps()]
            case p.SimpleEvent(name="ADAPTER_REMOVED"):
                self.adapter = None
                self.padmaps = []
                self.state = "NO_ADAPTER"
            case p.SimpleEvent():               # DUT_INSERTED, DUT_REMOVED, WRONG_ORIENTATION
                update.send.append(p.cmd_get_adapter())
            case p.EolWarning():
                update.send.append(p.cmd_get_adapter())
            case p.Fault():
                self.state = "FAULT"
                self.fault = msg.msg
            case p.TestStart():
                self.state = "TESTING"
                self._test_running = True
            case p.Summary() if self._test_running:
                self._test_running = False
                self.state = msg.outcome         # PASS or FAIL
                update.send.append(p.cmd_get_adapter())   # refresh the ins/tests counters
            case p.PadmapEntry():
                self._padmaps_pending.append(msg)
            case p.Ok(command="LIST_PADMAPS"):
                self.padmaps, self._padmaps_pending = self._padmaps_pending, []
            case p.Ok(command="SET_PADMAP" | "SET_DEFAULT_PADMAP"):
                update.send += [p.cmd_get_adapter(), p.cmd_list_padmaps()]
            case p.Error():
                self.last_error = msg
                self._padmaps_pending = []
                if msg.code in _NO_ADAPTER_ERRORS:
                    self.adapter = None
                    self.padmaps = []
        return update

"""Collects the protocol messages of one test into a TestResult.

A test arrives as a sequence of lines:

    EVENT TEST_START ...    test settings (drive currents, threshold, pad map)
    SLOT ...                per DUT slot
    PAD ...                 one per measured pad
    SUMMARY ...             pass/fail; ends the result

GET_RESULTS replays SLOT/PAD/SUMMARY without a TEST_START; the replayed result
then reuses the settings of the last TEST_START seen, if any.
"""

from __future__ import annotations

from collections.abc import Callable
from datetime import datetime

from . import protocol as p
from .die_shapes import shape_for_padmap
from .models import BondResult, PadResult, PadRole, TestContext, TestResult


class ResultAssembler:
    """Feed it every parsed message; it returns a TestResult on each SUMMARY.

    Besides test lines it also watches HELLO (tester identity) and PADMAP
    (pad map names), so results carry that context without extra plumbing.
    """

    def __init__(self, clock: Callable[[], datetime] = datetime.now) -> None:
        self._clock = clock               # timestamp source; injectable for tests
        self._tester: p.Hello | None = None
        self._padmap_names: dict[int, str] = {}
        self._last_context: TestContext | None = None

        # The result being collected. _collecting is False between results.
        self._collecting = False
        self._context: TestContext | None = None
        self._slot = 0
        self._pads: list[PadResult] = []

    def reset(self) -> None:
        """Drop a half-received result, e.g. after the connection was lost."""
        self._collecting = False
        self._pads = []

    def feed(self, msg: p.Message | None) -> TestResult | None:
        """Process one message. Returns the finished result on SUMMARY, else None."""
        match msg:
            case p.Hello():
                self._tester = msg
            case p.PadmapEntry():
                self._padmap_names[msg.id] = msg.name
            case p.TestStart():
                self._last_context = self._context_from(msg)
                self._begin(self._last_context)
            case p.Slot():
                if not self._collecting:      # GET_RESULTS replay
                    self._begin(self._last_context)
                self._slot = msg.slot
            case p.Pad() if self._collecting:
                self._pads.append(self._pad_result(msg))
            case p.Summary() if self._collecting:
                return self._finish(msg)
        return None

    # ——— internals ———————————————————————————————————————————————————————————

    def _begin(self, context: TestContext | None) -> None:
        self._collecting = True
        self._context = context
        self._slot = 0
        self._pads = []

    def _finish(self, summary: p.Summary) -> TestResult:
        # Millisecond precision, as in results files, so a live result and the
        # same result loaded from disk compare equal.
        now = self._clock()
        result = TestResult(
            ts=now.replace(microsecond=now.microsecond // 1000 * 1000),
            outcome=summary.outcome,
            good=summary.good,
            tested=summary.tested,
            reason=summary.reason,
            slot=self._slot,
            pads=self._pads,
            context=self._context,
        )
        self.reset()
        return result

    def _context_from(self, start: p.TestStart) -> TestContext:
        # Protocol v1 firmware has no active=; it then ran the adapter's default,
        # which is the first pm= entry.
        pm = start.active if start.active is not None else (start.pm[0] if start.pm else 0)
        return TestContext(
            aid=start.aid,
            ahw=start.ahw,
            ins=start.ins,
            tests=start.tests,
            pm=pm,
            pm_name=self._padmap_names.get(pm, ""),
            current_list_ua=start.current_list_ua,
            max_bond_r_ohms=start.max_bond_r_ohms,
            cap_time_list_us=start.cap_time_list_us,
            tester_uid=self._tester.uid if self._tester else "",
            tester_build=self._tester.build if self._tester else "",
        )

    def _pad_result(self, pad: p.Pad) -> PadResult:
        return PadResult(
            apin=pad.apin,
            dp=pad.dp,
            method=pad.method,
            result=BondResult.GOOD if pad.result == "GOOD" else BondResult.OPEN,
            role=self._role_of(pad),
            rf=pad.rf, rr=pad.rr, vf=pad.vf, vr=pad.vr, vfs=pad.vfs, vrs=pad.vrs,
        )

    def _role_of(self, pad: p.Pad) -> PadRole:
        """Pad role: the tester's type= if sent (protocol v2), else the die
        shape's entry for this pad, else IO."""
        if pad.type in PadRole.__members__:
            return PadRole(pad.type)
        shape = shape_for_padmap(self._context.pm) if self._context else None
        role = shape.role_of(pad.dp) if shape else None
        return role or PadRole.IO

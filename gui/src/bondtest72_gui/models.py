"""Result data model.

These types describe one completed test the same way the JSONL results file
does (see results_jsonl.py and docs/RESULTS_JSONL.md), so a result looks
identical whether it came straight off the serial port or was loaded from disk.
"""

from __future__ import annotations

from dataclasses import dataclass, field
from datetime import datetime
from enum import Enum


class BondResult(str, Enum):
    """Per-pad verdict reported by the tester."""

    GOOD = "GOOD"  # at least one reading showed conduction
    OPEN = "OPEN"  # no reading conducted — missing or broken bond


class PadRole(str, Enum):
    """Electrical role of a die pad.

    The first five come from the tester (type= on PAD lines). GND_BUS_ONLY and
    NC only appear in die shapes, for pads the tester never measures.
    """

    IO = "IO"
    VDDIO = "VDDIO"
    VDD_CORE = "VDD_CORE"
    PWR_AUX = "PWR_AUX"
    GND = "GND"
    GND_BUS_ONLY = "GND_BUS_ONLY"  # bonded to the substrate GND plane only, no adapter pin
    NC = "NC"                      # not connected / not tested

    @property
    def is_power(self) -> bool:
        return self in (PadRole.VDDIO, PadRole.VDD_CORE, PadRole.PWR_AUX)


@dataclass(frozen=True)
class PadResult:
    """Measurements for one die pad.

    `method` is STD/STD_FW/STD_REV or CAP/CAP_FW/CAP_REV. The suffix says which
    direction group(s) were measured; groups that weren't measured are None.

    STD pads carry three readings per direction, one per drive current
    (lowest current first — see TestContext.current_list_ua):
        rf / rr — apparent bond resistance in ohms (forward / reverse)
        vf / vr — the Kelvin voltages behind those resistances
    CAP pads carry a charging curve per direction instead, sampled at
    TestContext.cap_time_list_us:
        vfs / vrs — voltages, earliest sample first
    """

    apin: int
    dp: int
    method: str
    result: BondResult
    role: PadRole = PadRole.IO
    rf: list[float] | None = None
    rr: list[float] | None = None
    vf: list[float] | None = None
    vr: list[float] | None = None
    vfs: list[float] | None = None
    vrs: list[float] | None = None

    @property
    def is_cap(self) -> bool:
        """True for charging-curve (CAP_SENSE) pads, False for resistance sweeps."""
        return self.method.startswith("CAP")

    def volts(self, forward: bool) -> list[float] | None:
        """One direction's voltages: Kelvin voltage per drive current for STD
        pads, the charging curve for CAP pads; None if not measured."""
        if self.is_cap:
            return self.vfs if forward else self.vrs
        return self.vf if forward else self.vr


@dataclass(frozen=True)
class TestContext:
    """Tester, adapter and measurement settings in effect for one test."""

    __test__ = False  # not a pytest test class, despite the name

    aid: str = ""          # adapter serial (AT21CS01 UID)
    ahw: int = 0           # adapter hardware ID
    ins: int = 0           # adapter insertion count when the test started
    tests: int = 0         # adapter test count when the test started
    pm: int = 0            # pad map the test ran with
    pm_name: str = ""      # its human-readable name, when known
    current_list_ua: list[float] = field(default_factory=list)
    max_bond_r_ohms: float | None = None
    cap_time_list_us: list[int] | None = None
    tester_uid: str = ""
    tester_build: str = ""


@dataclass(frozen=True)
class TestResult:
    """One completed test of one DUT slot."""

    __test__ = False

    ts: datetime                 # when the result was recorded
    outcome: str                 # "PASS" or "FAIL"
    good: int                    # pads that passed
    tested: int                  # pads measured
    reason: str | None           # extra failure cause, e.g. "DUT_REMOVED"
    slot: int
    pads: list[PadResult]
    context: TestContext | None = None
    panel_id: str | None = None  # sample label entered by the user (factory: panel ID)
    img: str | None = None       # photo path relative to the results folder (factory only)
    thumb: str | None = None     # thumbnail path (factory only)

    @property
    def passed(self) -> bool:
        return self.outcome == "PASS"

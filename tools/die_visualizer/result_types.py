from dataclasses import dataclass, field
from enum import Enum


class BondResult(Enum):
    GOOD = "GOOD"
    OPEN = "OPEN"
    SHORT_GND = "SHORT_GND"


class PadRole(Enum):
    IO = "IO"
    VDDIO = "VDDIO"
    VDD_CORE = "VDD_CORE"
    PWR_AUX = "PWR_AUX"
    GND = "GND"
    GND_BUS_ONLY = "GND_BUS_ONLY"
    NC = "NC"


@dataclass
class PadResult:
    apin: int
    die_pad: int
    bond: BondResult
    prev_short: bool
    next_short: bool
    sense_v: float
    prev_v: float
    next_v: float
    role: PadRole = PadRole.IO


@dataclass
class SlotResult:
    slot: int
    present: bool
    tested: bool
    pads: list = field(default_factory=list)


@dataclass
class TesterInfo:
    name: str = ""
    build: str = ""
    uid: str = ""


@dataclass
class AdapterInfo:
    uid: str = ""
    hw: str = ""
    padmaps: list = field(default_factory=list)
    lifespan: int = 0
    mfg_date: str = ""
    ins: int = 0
    tests: int = 0
    eol: bool = False
    dut_present: bool = False


@dataclass
class TestResult:
    outcome: str
    good_count: int
    tested_count: int
    slots: list = field(default_factory=list)
    tester: TesterInfo = field(default_factory=TesterInfo)
    adapter: AdapterInfo = field(default_factory=AdapterInfo)
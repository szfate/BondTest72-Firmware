"""BondTest72 serial protocol: parse tester lines and build commands.

The protocol is line-based ASCII: a message keyword (``PAD``, ``EVENT TEST_START``,
...) followed by space-separated ``key=value`` fields. It is specified in
docs/BONDTEST72_HOST_PROTOCOL.md; this module follows that document.

``parse_line()`` turns one received line into a typed message object. It never
raises: firmware log lines become :class:`LogLine`, and anything unrecognised
or malformed becomes ``None``, because the same serial channel carries both
protocol and free-form logs. Unknown ``key=value`` fields are ignored, so newer
firmware that adds fields keeps working.
"""

from __future__ import annotations

import re
from dataclasses import dataclass

# Protocol version this module implements. Tester firmware that reports a lower
# proto= lacks some commands (see the version table in the protocol doc).
PROTOCOL_VERSION = 3

# PAD measurement methods; the _FW/_REV suffix says which direction(s) the line carries.
PAD_METHODS = frozenset({"STD", "STD_FW", "STD_REV", "CAP", "CAP_FW", "CAP_REV"})

# Tester states as reported by state= (ADAPTER) — see the protocol doc's state machine.
STATES = frozenset({
    "NO_ADAPTER", "EOL_ADAPTER", "ADAPTER_DETECTED", "READY",
    "WRONG_ORIENTATION", "TESTING", "PASS", "FAIL", "FAULT",
})


# ——— Messages ———————————————————————————————————————————————————————————————

@dataclass(frozen=True)
class Hello:
    """Reply to HELLO."""
    name: str
    build: str           # firmware commit date + short hash
    uid: str             # tester board ID
    proto: int = 1       # protocol version; firmware before v2 doesn't send it


@dataclass(frozen=True)
class AdapterInfo:
    """Reply to GET_ADAPTER."""
    aid: str
    ahw: int
    pm: list[int]          # pad map IDs in the adapter EEPROM, default first
    active: int | None     # pad map the next test runs with (v2+)
    lifespan: int
    mfg_date: int
    ins: int
    tests: int
    eol: bool
    dut: bool              # DUT present
    state: str | None      # tester state (v2+)


@dataclass(frozen=True)
class AdapterDetected:
    """EVENT ADAPTER_DETECTED — an adapter was plugged in (or re-initialised)."""
    aid: str
    ahw: int
    pm: list[int]
    active: int | None


@dataclass(frozen=True)
class TestStart:
    """EVENT TEST_START — a test began; PAD lines for it follow."""
    __test__ = False
    aid: str
    ahw: int
    ins: int
    tests: int
    pm: list[int]
    active: int | None
    current_list_ua: list[float]
    max_bond_r_ohms: float | None
    cap_time_list_us: list[int] | None


@dataclass(frozen=True)
class SimpleEvent:
    """An EVENT without fields: ADAPTER_REMOVED, DUT_INSERTED, DUT_REMOVED,
    WRONG_ORIENTATION."""
    name: str


@dataclass(frozen=True)
class EolWarning:
    """EVENT EOL_WARNING — the adapter has reached its insertion lifespan."""
    ins: int


@dataclass(frozen=True)
class Fault:
    """EVENT FAULT — the tester entered its FAULT state."""
    msg: str


@dataclass(frozen=True)
class PadmapEntry:
    """One line of a LIST_PADMAPS reply."""
    id: int
    pads: int
    default: bool
    active: bool
    name: str


@dataclass(frozen=True)
class Ok:
    """OK <command> [id=<n>] — a command succeeded."""
    command: str
    id: int | None = None


@dataclass(frozen=True)
class Error:
    """ERROR code=<n> msg=<text> — a command was rejected."""
    code: int
    msg: str


@dataclass(frozen=True)
class Slot:
    """SLOT — precedes one DUT slot's PAD lines."""
    slot: int
    present: bool
    tested: bool


@dataclass(frozen=True)
class Pad:
    """PAD — measurements for one die pad. See models.PadResult for the fields."""
    slot: int
    apin: int
    dp: int
    type: str | None        # pad role (v2+): IO, VDDIO, VDD_CORE, PWR_AUX, GND
    method: str
    result: str             # GOOD or OPEN
    rf: list[float] | None = None
    rr: list[float] | None = None
    vf: list[float] | None = None
    vr: list[float] | None = None
    vfs: list[float] | None = None
    vrs: list[float] | None = None


@dataclass(frozen=True)
class Summary:
    """SUMMARY — the last line of a test result."""
    outcome: str            # PASS or FAIL
    good: int
    tested: int
    reason: str | None      # fail_reason, e.g. DUT_REMOVED


@dataclass(frozen=True)
class LogLine:
    """A firmware log line, e.g. ``[  1234] [INFO] adapter: init ok``."""
    millis: int
    level: str              # DEBUG, INFO, WARN, ERROR
    text: str


Message = (
    Hello | AdapterInfo | AdapterDetected | TestStart | SimpleEvent | EolWarning
    | Fault | PadmapEntry | Ok | Error | Slot | Pad | Summary | LogLine
)


# ——— Parsing ————————————————————————————————————————————————————————————————

class _Malformed(Exception):
    """A recognised message with a missing or unparsable required field."""


_KV = re.compile(r"(\w+)=(\S*)")
_LOG = re.compile(r"^\[\s*(\d+)\]\s+\[(\w+)\]\s?(.*)$")
_SIMPLE_EVENTS = frozenset({"ADAPTER_REMOVED", "DUT_INSERTED", "DUT_REMOVED", "WRONG_ORIENTATION"})


def _fields(text: str) -> dict[str, str]:
    """All key=value pairs in `text`."""
    return dict(_KV.findall(text))


def _int(d: dict[str, str], key: str, default: int | None = None) -> int:
    if key not in d:
        if default is None:
            raise _Malformed(key)
        return default
    try:
        return int(d[key])
    except ValueError:
        raise _Malformed(key) from None


def _opt_int(d: dict[str, str], key: str) -> int | None:
    return _int(d, key) if key in d else None


def _opt_float(d: dict[str, str], key: str) -> float | None:
    if key not in d:
        return None
    try:
        return float(d[key])
    except ValueError:
        raise _Malformed(key) from None


def _float_list(d: dict[str, str], key: str) -> list[float] | None:
    """Comma-separated floats, or None when the field is absent."""
    if key not in d:
        return None
    try:
        return [float(v) for v in d[key].split(",")]
    except ValueError:
        raise _Malformed(key) from None


def _int_list(d: dict[str, str], key: str) -> list[int] | None:
    if key not in d:
        return None
    try:
        return [int(v) for v in d[key].split(",")]
    except ValueError:
        raise _Malformed(key) from None


def _flag(d: dict[str, str], key: str) -> bool:
    return d.get(key) == "1"


def _parse_hello(rest: str) -> Hello:
    d = _fields(rest)
    return Hello(name=d.get("name", ""), build=d.get("build", ""), uid=d.get("uid", ""),
                 proto=_int(d, "proto", default=1))


def _parse_adapter(rest: str) -> AdapterInfo:
    d = _fields(rest)
    return AdapterInfo(
        aid=d.get("aid", ""), ahw=_int(d, "ahw"), pm=_int_list(d, "pm") or [],
        active=_opt_int(d, "active"), lifespan=_int(d, "lifespan", 0),
        mfg_date=_int(d, "mfg_date", 0), ins=_int(d, "ins", 0), tests=_int(d, "tests", 0),
        eol=_flag(d, "eol"), dut=_flag(d, "dut"), state=d.get("state"),
    )


def _parse_event(rest: str) -> Message | None:
    name, _, args = rest.partition(" ")
    d = _fields(args)
    if name in _SIMPLE_EVENTS:
        return SimpleEvent(name)
    if name == "ADAPTER_DETECTED":
        return AdapterDetected(aid=d.get("aid", ""), ahw=_int(d, "ahw"),
                               pm=_int_list(d, "pm") or [], active=_opt_int(d, "active"))
    if name == "TEST_START":
        return TestStart(
            aid=d.get("aid", ""), ahw=_int(d, "ahw"), ins=_int(d, "ins", 0),
            tests=_int(d, "tests", 0), pm=_int_list(d, "pm") or [],
            active=_opt_int(d, "active"),
            current_list_ua=_float_list(d, "current_list_ua") or [],
            max_bond_r_ohms=_opt_float(d, "max_bond_r_ohms"),
            cap_time_list_us=_int_list(d, "cap_time_list_us"),
        )
    if name == "EOL_WARNING":
        return EolWarning(ins=_int(d, "ins"))
    if name == "FAULT":
        return Fault(msg=d.get("msg", ""))
    return None


def _parse_padmap(rest: str) -> PadmapEntry:
    # name= is always last and runs to the end of the line (names contain spaces),
    # so split it off before reading the ordinary key=value fields.
    head, sep, name = rest.partition(" name=")
    if not sep:
        head, name = (rest[len("name="):], "") if rest.startswith("name=") else (rest, "")
    d = _fields(head)
    return PadmapEntry(id=_int(d, "id"), pads=_int(d, "pads", 0),
                       default=_flag(d, "default"), active=_flag(d, "active"), name=name)


def _parse_ok(rest: str) -> Ok:
    command, _, args = rest.partition(" ")
    return Ok(command=command, id=_opt_int(_fields(args), "id"))


def _parse_error(rest: str) -> Error:
    d = _fields(rest)
    return Error(code=_int(d, "code"), msg=d.get("msg", ""))


def _parse_slot(rest: str) -> Slot:
    d = _fields(rest)
    return Slot(slot=_int(d, "slot"), present=_flag(d, "present"), tested=_flag(d, "tested"))


def _parse_pad(rest: str) -> Pad | None:
    d = _fields(rest)
    if d.get("method") not in PAD_METHODS:
        return None
    return Pad(
        slot=_int(d, "slot", 0), apin=_int(d, "apin"), dp=_int(d, "dp"),
        type=d.get("type"), method=d["method"], result=d.get("result", ""),
        rf=_float_list(d, "rf"), rr=_float_list(d, "rr"),
        vf=_float_list(d, "vf"), vr=_float_list(d, "vr"),
        vfs=_float_list(d, "vfs"), vrs=_float_list(d, "vrs"),
    )


def _parse_summary(rest: str) -> Summary:
    d = _fields(rest)
    return Summary(outcome=d.get("outcome", ""), good=_int(d, "good"),
                   tested=_int(d, "tested"), reason=d.get("fail_reason") or None)


# Message keyword → parser for the rest of the line.
_PARSERS = {
    "HELLO": _parse_hello,
    "ADAPTER": _parse_adapter,
    "EVENT": _parse_event,
    "PADMAP": _parse_padmap,
    "OK": _parse_ok,
    "ERROR": _parse_error,
    "SLOT": _parse_slot,
    "PAD": _parse_pad,
    "SUMMARY": _parse_summary,
}


def parse_line(line: str) -> Message | None:
    """Parse one line received from the tester.

    Returns the matching message object, a LogLine for firmware log output, or
    None for blank, unknown or malformed lines.
    """
    line = line.strip()
    if not line:
        return None

    log = _LOG.match(line)
    if log:
        return LogLine(millis=int(log.group(1)), level=log.group(2), text=log.group(3))

    keyword, _, rest = line.partition(" ")
    parser = _PARSERS.get(keyword)
    if parser is None:
        return None
    try:
        return parser(rest)
    except _Malformed:
        return None


# ——— Commands ———————————————————————————————————————————————————————————————
# Each returns the full line to write to the port, newline included.

def cmd_hello() -> str:
    return "HELLO\n"


def cmd_get_adapter() -> str:
    return "GET_ADAPTER\n"


def cmd_list_padmaps() -> str:
    return "LIST_PADMAPS\n"


def cmd_set_padmap(padmap_id: int) -> str:
    return f"SET_PADMAP id={padmap_id}\n"


def cmd_set_default_padmap(padmap_id: int) -> str:
    return f"SET_DEFAULT_PADMAP id={padmap_id}\n"


def cmd_run() -> str:
    return "RUN\n"


def cmd_get_results() -> str:
    return "GET_RESULTS\n"

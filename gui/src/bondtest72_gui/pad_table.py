"""Per-pad table of one test result, for one pad group (IO or power).

Each voltage reading gets its own column, headed with what it was measured
at: the drive current for IO pads (resistance sweep), the sample time for
power pads (charging curve). Cells hold the raw value under SORT_ROLE so a
sort proxy orders them numerically.
"""

from __future__ import annotations

from dataclasses import dataclass
from enum import Enum

from PySide6.QtCore import QAbstractTableModel, QModelIndex, Qt
from PySide6.QtGui import QBrush

from .die_view import GOOD_COLOR, OPEN_COLOR
from .formatting import format_current, format_time, format_volts, pad_summary
from .models import BondResult, PadResult, TestResult

SORT_ROLE = Qt.ItemDataRole.UserRole

BASE_COLUMNS = ("Pad", "Pin", "Role", "Result")


class PadGroup(Enum):
    IO = "IO pads"          # resistance sweep (STD)
    POWER = "Power pads"    # charging curve (CAP): the bypass-capped supply pads

    def includes(self, pad: PadResult) -> bool:
        # Grouped by method, not role: older results files label every pad IO.
        return pad.is_cap == (self is PadGroup.POWER)


def group_pads(result: TestResult | None, group: PadGroup) -> list[PadResult]:
    return [pad for pad in result.pads if group.includes(pad)] if result else []


def reading_levels(result: TestResult | None, group: PadGroup) -> list[str]:
    """Labels for the readings: drive currents (IO) or sample times (power)."""
    ctx = result.context if result else None
    if ctx is None:
        return []
    if group is PadGroup.IO:
        return [format_current(ua) for ua in ctx.current_list_ua]
    return [format_time(us) for us in ctx.cap_time_list_us or []]


@dataclass(frozen=True)
class _ReadingColumn:
    forward: bool
    index: int              # position in the pad's voltage list
    header: str


class PadTableModel(QAbstractTableModel):
    """One row per pad of the group, in the order the tester reported them."""

    def __init__(self, group: PadGroup) -> None:
        super().__init__()
        self.group = group
        self._pads: list[PadResult] = []
        self._readings: list[_ReadingColumn] = []

    def set_result(self, result: TestResult | None) -> None:
        self.beginResetModel()
        self._pads = group_pads(result, self.group)
        levels = reading_levels(result, self.group)
        self._readings = []
        for forward, prefix in ((False, "Rev"), (True, "Fwd")):
            count = max((len(pad.volts(forward) or []) for pad in self._pads), default=0)
            for i in range(count):
                level = levels[i] if i < len(levels) else f"#{i + 1}"
                self._readings.append(_ReadingColumn(forward, i, f"{prefix} {level}"))
        self.endResetModel()

    def row_of(self, die_pad: int) -> int | None:
        return next((i for i, pad in enumerate(self._pads) if pad.dp == die_pad), None)

    def die_pad_at(self, row: int) -> int | None:
        return self._pads[row].dp if 0 <= row < len(self._pads) else None

    def rowCount(self, parent: QModelIndex = QModelIndex()) -> int:
        return 0 if parent.isValid() else len(self._pads)

    def columnCount(self, parent: QModelIndex = QModelIndex()) -> int:
        return 0 if parent.isValid() else len(BASE_COLUMNS) + len(self._readings)

    def headerData(self, section: int, orientation: Qt.Orientation, role: int = Qt.ItemDataRole.DisplayRole):
        if orientation != Qt.Orientation.Horizontal or role != Qt.ItemDataRole.DisplayRole:
            return None
        if section < len(BASE_COLUMNS):
            return BASE_COLUMNS[section]
        return self._readings[section - len(BASE_COLUMNS)].header

    def data(self, index: QModelIndex, role: int = Qt.ItemDataRole.DisplayRole):
        if not index.isValid():
            return None
        pad = self._pads[index.row()]
        column = index.column()

        if role == Qt.ItemDataRole.ToolTipRole:
            return pad_summary(pad)
        if column < len(BASE_COLUMNS):
            name = BASE_COLUMNS[column]
            value = {"Pad": pad.dp, "Pin": pad.apin, "Role": pad.role.value, "Result": pad.result.value}[name]
            if role in (Qt.ItemDataRole.DisplayRole, SORT_ROLE):
                return value
            if role == Qt.ItemDataRole.ForegroundRole and name == "Result":
                return QBrush(GOOD_COLOR if pad.result is BondResult.GOOD else OPEN_COLOR)
            return None

        reading = self._readings[column - len(BASE_COLUMNS)]
        volts = pad.volts(reading.forward) or []
        value = volts[reading.index] if reading.index < len(volts) else None
        if role == Qt.ItemDataRole.DisplayRole:
            return format_volts(value) if value is not None else ""
        if role == SORT_ROLE:
            return value if value is not None else float("inf")
        if role == Qt.ItemDataRole.TextAlignmentRole:
            return int(Qt.AlignmentFlag.AlignRight | Qt.AlignmentFlag.AlignVCenter)
        return None

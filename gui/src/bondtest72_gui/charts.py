"""Graphs of one result: IO pad voltages and power pad charging curves.

IoChart puts every IO pad side by side (x = die pad number, so pads that
aren't IO-measured leave gaps) with one series per drive current, so a bond
that reads differently from its neighbours stands out. PowerChart overlays the charging curve of each power
pad (x = time since the charge started, log scale).

Reverse readings have filled markers, forward readings hollow ones (and dashed
lines on the charging curves), so direction never relies on colour alone.
"""

from __future__ import annotations

import math

import pyqtgraph as pg
from PySide6.QtCore import Qt, Signal
from PySide6.QtGui import QColor, QPalette
from PySide6.QtWidgets import QApplication, QLabel, QVBoxLayout, QWidget

from .die_view import OPEN_COLOR
from .formatting import format_current, format_time, format_volts
from .models import BondResult, PadResult, TestResult
from .pad_table import PadGroup, group_pads

# Categorical slots (validated light/dark pairs). Drive currents use the first
# three, which stay distinguishable even as a scatter for colour-blind readers.
_SLOTS_LIGHT = ["#2a78d6", "#eb6834", "#1baf7a", "#eda100", "#e87ba4", "#008300", "#6250d6", "#e34948"]
_SLOTS_DARK = ["#3987e5", "#d95926", "#199e70", "#c98500", "#d55181", "#008300", "#9085e9", "#e66767"]
# Power pads get one hue each, skipping green and red (they read as pass/fail).
_PAD_SLOTS = [0, 1, 2, 3, 4, 6]

TESTER_MAX_V = 3.3   # pull-up rail: an open bond reads at or near this
Y_TOP = 3.5          # just above the rail (the legend sits outside the plot)
IO_MIN_TOP = 1.0     # IO charts: good bonds sit below this; higher readings stretch the view


def _io_y_top(pads: list[PadResult]) -> float:
    """Top of the IO charts' voltage axis: 0–1 V shows good bonds in detail,
    and any higher reading stretches the view to include it (up to just above
    the rail)."""
    peak = max((v for pad in pads for fwd in (False, True) for v in pad.volts(fwd) or []), default=0.0)
    return min(Y_TOP, max(IO_MIN_TOP, peak * 1.05))


def _is_dark() -> bool:
    app = QApplication.instance()
    return isinstance(app, QApplication) and app.palette().color(QPalette.ColorRole.Window).lightness() < 128


def _slot(i: int) -> QColor:
    return QColor((_SLOTS_DARK if _is_dark() else _SLOTS_LIGHT)[i])


class _Chart(QWidget):
    """Shared look: a wrapping legend line above a palette-matched plot with a
    recessive grid; clicking a marker selects its pad."""

    pad_clicked = Signal(int)

    def __init__(self, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.plot = pg.PlotWidget()
        pal = self.palette()
        self.plot.setBackground(pal.color(QPalette.ColorRole.Base))
        text = pal.color(QPalette.ColorRole.Text)
        muted = pal.color(QPalette.ColorRole.PlaceholderText)
        item = self.getPlotItem()
        for name in ("left", "bottom"):
            axis = item.getAxis(name)
            axis.setPen(muted)
            axis.setTextPen(text)
        item.showGrid(x=False, y=True, alpha=0.15)
        item.setMenuEnabled(False)
        item.hideButtons()

        # The legend lives outside the plot, so it never covers points near the
        # 3.3 V rail, and wraps instead of widening the chart.
        self._legend: list[str] = []
        self._legend_label = QLabel(wordWrap=True, textFormat=Qt.TextFormat.RichText)
        self._legend_label.setContentsMargins(8, 4, 8, 0)
        self._legend_label.setAutoFillBackground(True)
        self._legend_label.setBackgroundRole(QPalette.ColorRole.Base)

        layout = QVBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(0)
        layout.addWidget(self._legend_label)
        layout.addWidget(self.plot, 1)
        self.setMinimumHeight(160)
        self._selected: int | None = None

    def getPlotItem(self) -> pg.PlotItem:
        return self.plot.getPlotItem()

    def addItem(self, item) -> None:
        self.plot.addItem(item)

    def removeItem(self, item) -> None:
        self.plot.removeItem(item)

    def set_selected(self, die_pad: int | None) -> None:
        if die_pad != self._selected:
            self._selected = die_pad
            self._redraw_selection()

    def _redraw_selection(self) -> None:
        raise NotImplementedError

    def _clear(self) -> None:
        self.getPlotItem().clear()
        self._legend = []
        self._legend_label.clear()

    def _add_legend(self, marker: str, color: QColor, label: str) -> None:
        """One legend entry: a coloured marker glyph, then its label in text colour."""
        self._legend.append(f'<span style="color:{color.name()}">{marker}</span>&nbsp;{label}')

    def _show_legend(self) -> None:
        # Entries are kept whole (&nbsp; inside) and wrap between each other.
        self._legend_label.setText(" &nbsp; ".join(self._legend))

    def _points(self, x, y, color: QColor, forward: bool, data, tip, size: float = 8) -> pg.ScatterPlotItem:
        """Markers that show a tooltip on hover and select their pad on click."""
        surface = self.palette().color(QPalette.ColorRole.Base)
        scatter = pg.ScatterPlotItem(
            x=x, y=y, data=data, size=size, symbol="o",
            pen=pg.mkPen(color if forward else surface, width=1.5),
            brush=pg.mkBrush(surface if forward else color),
            hoverable=True, hoverSize=size + 4, tip=tip,
        )
        scatter.sigClicked.connect(self._points_clicked)
        self.addItem(scatter)
        return scatter

    def _points_clicked(self, _item, points, _event) -> None:
        # `points` is a numpy array of SpotItems (several when markers overlap),
        # so test its length rather than its truth value.
        if len(points):
            self.pad_clicked.emit(points[0].data()[0])

    def _line(self, x, y, color: QColor, forward: bool, width: float = 2) -> pg.PlotCurveItem:
        style = Qt.PenStyle.DashLine if forward else Qt.PenStyle.SolidLine
        curve = pg.PlotCurveItem(x=x, y=y, pen=pg.mkPen(color, width=width, style=style))
        self.addItem(curve)
        return curve


class IoChart(_Chart):
    """Voltage across every IO pad, one series per drive current."""

    def __init__(self, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        item = self.getPlotItem()
        item.setLabel("left", "Voltage", units="V")
        item.setLabel("bottom", "Die pad")
        item.setMouseEnabled(x=True, y=False)
        self._pads: list[PadResult] = []
        self._selection_line: pg.InfiniteLine | None = None

    def set_result(self, result: TestResult | None) -> None:
        self._clear()
        self._pads = group_pads(result, PadGroup.IO)
        self._selection_line = None
        if not self._pads:
            return
        currents = result.context.current_list_ua if result and result.context else []

        # Open bonds: a faint status band behind the pad, named in the legend.
        opens = [pad.dp for pad in self._pads if pad.result is BondResult.OPEN]
        if opens:
            band = QColor(OPEN_COLOR)
            band.setAlpha(45)
            bars = pg.BarGraphItem(x=opens, height=Y_TOP, width=0.8, y0=0,
                                   brush=band, pen=pg.mkPen(None))
            self.addItem(bars)
            self._add_legend("■", OPEN_COLOR, "OPEN bond")

        in_legend: set[int] = set()
        for forward in (False, True):
            levels = max((len(pad.volts(forward) or []) for pad in self._pads), default=0)
            for level in range(levels):
                xs, ys, data = [], [], []
                for pad in self._pads:
                    volts = pad.volts(forward) or []
                    if level < len(volts):
                        xs.append(pad.dp)
                        ys.append(volts[level])
                        data.append((pad.dp, level, forward))
                color = _slot(level)
                label = format_current(currents[level]) if level < len(currents) else f"#{level + 1}"
                self._points(xs, ys, color, forward, data, self._tip(currents))
                if level not in in_legend:          # one entry per current, either direction
                    in_legend.add(level)
                    self._add_legend("●", color, label)

        if any(pad.volts(True) for pad in self._pads):
            ink = self._legend_label.palette().color(QPalette.ColorRole.Text)
            self._add_legend("●", ink, "reverse")
            self._add_legend("○", ink, "forward")
        self._show_legend()
        self._set_pad_ticks()
        self.getPlotItem().setYRange(0, _io_y_top(self._pads), padding=0)
        dps = [pad.dp for pad in self._pads]
        self.getPlotItem().setXRange(min(dps) - 0.8, max(dps) + 0.8, padding=0)
        self._redraw_selection()

    def _tip(self, currents: list[float]):
        def tip(x, y, data) -> str:
            dp, level, forward = data
            label = format_current(currents[level]) if level < len(currents) else f"reading {level + 1}"
            return f"Die pad {dp} · {label} {'forward' if forward else 'reverse'}\n{format_volts(y)} V"
        return tip

    def _set_pad_ticks(self) -> None:
        # Labelled tick every 5 die pads, a small tick at every pad position.
        last = max(pad.dp for pad in self._pads)
        major = [(dp, str(dp)) for dp in range(0, last + 1, 5)]
        minor = [(dp, "") for dp in range(0, last + 1) if dp % 5]
        self.getPlotItem().getAxis("bottom").setTicks([major, minor])

    def _redraw_selection(self) -> None:
        if self._selection_line is not None:
            self.removeItem(self._selection_line)
            self._selection_line = None
        x = self._selected if any(pad.dp == self._selected for pad in self._pads) else None
        if x is not None:
            pen = pg.mkPen(self.palette().color(QPalette.ColorRole.Highlight), width=2)
            self._selection_line = pg.InfiniteLine(pos=x, angle=90, pen=pen)
            self.addItem(self._selection_line)


class IoOverlayChart(_Chart):
    """Every IO pad's curve on one graph: voltage against drive current.

    A good bond traces the same diode-like curve as its neighbours, so a pad
    whose curve leaves the bundle stands out even when it still passes. With
    dozens of curves, colour shows status (GOOD / OPEN), not pad identity;
    hover a point or select a pad to see which one it is.
    """

    def __init__(self, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        item = self.getPlotItem()
        item.setLabel("left", "Voltage", units="V")
        item.setLabel("bottom", "Nominal drive current (log scale)")
        item.setMouseEnabled(x=False, y=False)
        self._curves: dict[int, list[tuple[pg.PlotCurveItem, QColor, bool]]] = {}

    def set_result(self, result: TestResult | None) -> None:
        self._clear()
        self._curves = {}
        pads = group_pads(result, PadGroup.IO)
        currents = result.context.current_list_ua if result and result.context else []
        if not pads or not currents:
            return
        log_i = [math.log10(i) for i in currents]
        self.getPlotItem().getAxis("bottom").setTicks([[(x, format_current(i)) for x, i in zip(log_i, currents)], []])

        good = _slot(0)
        good.setAlpha(110)                 # many overlapping curves: let density show
        any_forward = False
        # GOOD first so OPEN curves draw on top.
        for pad in sorted(pads, key=lambda p: p.result is BondResult.OPEN):
            color = good if pad.result is BondResult.GOOD else QColor(OPEN_COLOR)
            for forward in (False, True):
                volts = pad.volts(forward)
                if not volts:
                    continue
                any_forward |= forward
                xs = log_i[:len(volts)]
                curve = self._line(xs, volts, color, forward, width=1.2)
                self._curves.setdefault(pad.dp, []).append((curve, color, forward))
                data = [(pad.dp, i, forward) for i in currents[:len(volts)]]
                self._points(xs, volts, color, forward, data, self._tip, size=5)

        self._add_legend("━━", _slot(0), "GOOD pad")
        if any(pad.result is BondResult.OPEN for pad in pads):
            self._add_legend("━━", OPEN_COLOR, "OPEN pad")
        if any_forward:
            ink = self._legend_label.palette().color(QPalette.ColorRole.Text)
            self._add_legend("━━", ink, "reverse")
            self._add_legend("╍╍", ink, "forward")
        self._show_legend()
        # Extra room on the right so the last tick label isn't clipped.
        self.getPlotItem().setXRange(min(log_i) - 0.2, max(log_i) + 0.35, padding=0)
        self.getPlotItem().setYRange(0, _io_y_top(pads), padding=0)
        self._redraw_selection()

    @staticmethod
    def _tip(x, y, data) -> str:
        dp, current, forward = data
        return f"Die pad {dp} · {format_current(current)} {'forward' if forward else 'reverse'}\n{format_volts(y)} V"

    def _redraw_selection(self) -> None:
        highlight = self.palette().color(QPalette.ColorRole.Highlight)
        for dp, curves in self._curves.items():
            for curve, color, forward in curves:
                selected = dp == self._selected
                style = Qt.PenStyle.DashLine if forward else Qt.PenStyle.SolidLine
                curve.setPen(pg.mkPen(highlight if selected else color, width=3 if selected else 1.2, style=style))
                curve.setZValue(10 if selected else 0)


class PowerChart(_Chart):
    """Charging curve of every power pad, one line per pad."""

    def __init__(self, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        item = self.getPlotItem()
        item.setLabel("left", "Voltage", units="V")
        item.setLabel("bottom", "Time since charge start (log scale)")
        self._curves: dict[int, list[pg.PlotCurveItem]] = {}

    def set_result(self, result: TestResult | None) -> None:
        self._clear()
        self._curves = {}
        pads = group_pads(result, PadGroup.POWER)
        times = result.context.cap_time_list_us if result and result.context else None
        if not pads or not times:
            return
        # x is log10(time): the samples span 156 µs to 20 ms. Plotted log values
        # directly (rather than pyqtgraph's log mode, which only transforms
        # PlotDataItems); ticks show the real sample times.
        self.getPlotItem().getAxis("bottom").setTicks(
            [[(math.log10(t), format_time(t)) for t in times], []])
        for n, pad in enumerate(pads):
            # One hue per pad; past the six hues, repeat them dotted.
            color = _slot(_PAD_SLOTS[n % len(_PAD_SLOTS)])
            dotted = n >= len(_PAD_SLOTS)
            for forward in (False, True):
                volts = pad.volts(forward)
                if not volts:
                    continue
                ts = times[:len(volts)]
                xs = [math.log10(t) for t in ts]
                curve = self._line(xs, volts, color, forward)
                if dotted and not forward:
                    curve.setPen(pg.mkPen(color, width=2, style=Qt.PenStyle.DotLine))
                self._curves.setdefault(pad.dp, []).append(curve)
                data = [(pad.dp, t, forward) for t in ts]
                self._points(xs, volts, color, forward, data, self._tip)
                if not forward:
                    # Recorded roles can't be trusted here (older files say IO
                    # for every pad), and every pad on this chart is a supply.
                    status = "" if pad.result is BondResult.GOOD else " OPEN"
                    self._add_legend("┈┈" if dotted else "━━", color, f"pad {pad.dp}{status}")
        self._show_legend()
        log_t = [math.log10(t) for t in times]
        self.getPlotItem().setXRange(min(log_t) - 0.1, max(log_t) + 0.1, padding=0)
        # Scaled to the data: the curves often settle well below the rail.
        peak = max((v for pad in pads for fwd in (False, True) for v in pad.volts(fwd) or []), default=1.0)
        self.getPlotItem().setYRange(0, min(Y_TOP, max(1.0, peak * 1.15)), padding=0)
        self._redraw_selection()

    @staticmethod
    def _tip(x, y, data) -> str:
        dp, t, forward = data
        return f"Die pad {dp} · {'forward' if forward else 'reverse'}\n{format_volts(y)} V at {format_time(t)}"

    def _redraw_selection(self) -> None:
        for dp, curves in self._curves.items():
            for curve in curves:
                pen = curve.opts["pen"]
                pen = pg.mkPen(pen)
                pen.setWidthF(4 if dp == self._selected else 2)
                curve.setPen(pen)

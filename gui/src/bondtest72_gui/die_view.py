"""Die drawing: the die outline with its pad ring, each pad coloured by result."""

from __future__ import annotations

from dataclasses import dataclass

from PySide6.QtCore import QEvent, QPointF, QRectF, QSize, Qt, Signal
from PySide6.QtGui import QColor, QFont, QFontMetricsF, QHelpEvent, QMouseEvent, QPainter, QPalette, QPen
from PySide6.QtWidgets import QLabel, QSizePolicy, QToolTip, QWidget

from .die_shapes import DieShape, PadInfo
from .formatting import pad_summary
from .models import BondResult, PadResult, PadRole, TestResult
from .theme import is_dark

GOOD_COLOR = QColor("#2e9d4f")
OPEN_COLOR = QColor("#d43c3c")
UNTESTED_COLOR = QColor("#8a8f98")    # tested by this pad map, but no result yet
POWER_COLOR = QColor("#e0a020")       # outline for supply pads
GND_COLOR = QColor("#3d7fd9")         # outline for ground pads

# Die body: a light grey in the light theme, a mid grey in the dark one, so the
# black die name and the coloured pads read on it either way.
DIE_LIGHT = QColor("#e6e7ea")
DIE_DARK = QColor("#9a9da3")

LABEL_MARGIN = 26.0                   # room outside the die for pad numbers


@dataclass(frozen=True)
class PadPlacement:
    pad: PadInfo
    rect: QRectF
    side: str                         # top / left / bottom / right


def fit_die(shape: DieShape, area: QRectF, margin: float = LABEL_MARGIN) -> QRectF:
    """The largest rectangle with the die's aspect ratio centred in `area`."""
    inner = area.adjusted(margin, margin, -margin, -margin)
    if inner.width() <= 0 or inner.height() <= 0:
        return QRectF(area.center(), QSize(0, 0))
    w = min(inner.width(), inner.height() * shape.aspect_ratio)
    h = w / shape.aspect_ratio
    return QRectF(inner.center().x() - w / 2, inner.center().y() - h / 2, w, h)


def place_pads(shape: DieShape, die: QRectF) -> list[PadPlacement]:
    """Pad rectangles along the inside of the die edge, in ring order.

    Ring order (see die_shapes): top right→left, left top→bottom, bottom
    left→right, right bottom→top. Each side leaves its corners free so pads
    from neighbouring sides never overlap.
    """
    sides = shape.sides()
    densest = max(len(sides["top"]) / max(die.width(), 1), len(sides["left"]) / max(die.height(), 1),
                  len(sides["bottom"]) / max(die.width(), 1), len(sides["right"]) / max(die.height(), 1))
    depth = min(0.6 / densest if densest else 20.0, 0.12 * min(die.width(), die.height()))

    placements = []
    for side, pads in sides.items():
        n = len(pads)
        if n == 0:
            continue
        horizontal = side in ("top", "bottom")
        length = (die.width() if horizontal else die.height()) - 2 * depth
        pitch = length / n
        size = pitch * 0.72
        for i, pad in enumerate(pads):
            along = depth + (i + 0.5) * pitch            # distance from the side's start corner
            if side == "top":
                center = QPointF(die.right() - along, die.top() + depth / 2)
            elif side == "left":
                center = QPointF(die.left() + depth / 2, die.top() + along)
            elif side == "bottom":
                center = QPointF(die.left() + along, die.bottom() - depth / 2)
            else:
                center = QPointF(die.right() - depth / 2, die.bottom() - along)
            w, h = (size, depth * 0.8) if horizontal else (depth * 0.8, size)
            placements.append(PadPlacement(pad, QRectF(center.x() - w / 2, center.y() - h / 2, w, h), side))
    return placements


class DieView(QWidget):
    """Shows one die shape and, optionally, a result on it. Click a pad to select it."""

    pad_clicked = Signal(int)          # die pad number

    def __init__(self, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self._shape: DieShape | None = None
        self._results: dict[int, PadResult] = {}
        self._selected: int | None = None
        self._placements: list[PadPlacement] = []
        self._message = "No die selected"
        self.setMouseTracking(True)
        self.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Expanding)

    def sizeHint(self) -> QSize:
        return QSize(520, 560)

    def minimumSizeHint(self) -> QSize:
        return QSize(240, 240)

    def set_shape(self, shape: DieShape | None, message: str = "") -> None:
        """Die to draw. `message` is shown instead when `shape` is None."""
        self._shape = shape
        self._message = message
        self._placements = []
        self.update()

    def set_result(self, result: TestResult | None) -> None:
        self._results = {pad.dp: pad for pad in result.pads} if result else {}
        self.update()

    def set_selected(self, die_pad: int | None) -> None:
        if die_pad != self._selected:
            self._selected = die_pad
            self.update()

    def pad_at(self, pos: QPointF) -> int | None:
        for placement in self._placements:
            if placement.rect.adjusted(-2, -2, 2, 2).contains(pos):
                return placement.pad.die_pad
        return None

    # ——— painting ———————————————————————————————————————————————————————————

    def paintEvent(self, event) -> None:
        painter = QPainter(self)
        painter.setRenderHint(QPainter.RenderHint.Antialiasing)
        pal = self.palette()
        painter.fillRect(self.rect(), pal.color(QPalette.ColorRole.Base))

        if self._shape is None:
            painter.setPen(pal.color(QPalette.ColorRole.PlaceholderText))
            painter.drawText(self.rect(), Qt.AlignmentFlag.AlignCenter, self._message)
            return

        die = fit_die(self._shape, QRectF(self.rect()))
        self._placements = place_pads(self._shape, die)

        painter.setPen(QPen(pal.color(QPalette.ColorRole.Mid), 1.5))
        painter.setBrush(DIE_DARK if is_dark(pal) else DIE_LIGHT)
        painter.drawRoundedRect(die, 4, 4)

        # Die name: black on the light-grey die in both themes, a size up from body text.
        name_font = QFont(self.font())
        name_font.setPointSizeF(name_font.pointSizeF() * 1.5)
        painter.save()
        painter.setFont(name_font)
        painter.setPen(QColor(Qt.GlobalColor.black))
        painter.drawText(die, Qt.AlignmentFlag.AlignCenter, self._shape.name)
        painter.restore()

        for placement in self._placements:
            self._paint_pad(painter, placement)
        self._paint_labels(painter, die)

    def _paint_pad(self, painter: QPainter, placement: PadPlacement) -> None:
        pad = placement.pad
        result = self._results.get(pad.die_pad)
        unmeasured = pad.role in (PadRole.NC, PadRole.GND_BUS_ONLY)

        if result is not None:
            fill = GOOD_COLOR if result.result is BondResult.GOOD else OPEN_COLOR
        elif unmeasured or (self._results and pad.role is PadRole.GND):
            fill = QColor(Qt.GlobalColor.transparent)       # never measured
        else:
            fill = UNTESTED_COLOR

        # The die shape knows supply and ground pads; results from older tools
        # label every pad IO, so a recorded IO doesn't override the shape.
        role = result.role if result and result.role is not PadRole.IO else pad.role
        if role.is_power:
            outline = QPen(POWER_COLOR, 2)
        elif role in (PadRole.GND, PadRole.GND_BUS_ONLY):
            outline = QPen(GND_COLOR, 2)
        else:
            outline = QPen(self.palette().color(QPalette.ColorRole.Mid), 1)
        if pad.die_pad == self._selected:
            outline = QPen(self.palette().color(QPalette.ColorRole.Highlight), 3)

        painter.setPen(outline)
        painter.setBrush(fill)
        painter.drawRect(placement.rect)

    def _paint_labels(self, painter: QPainter, die: QRectF) -> None:
        font = QFont(self.font())
        font.setPointSizeF(max(6.0, font.pointSizeF() - 3))
        painter.setFont(font)
        metrics = QFontMetricsF(font)
        painter.setPen(self.palette().color(QPalette.ColorRole.Text))

        # Top and bottom labels stand upright when the side's widest number
        # fits between neighbouring pads, and turn sideways only when it doesn't.
        upright: dict[str, bool] = {}
        for side in ("top", "bottom"):
            on_side = [p for p in self._placements if p.side == side]
            if on_side:
                widest = max(metrics.horizontalAdvance(str(p.pad.die_pad)) for p in on_side)
                upright[side] = widest <= on_side[0].rect.width() / 0.72 * 0.9

        gap = 3.0
        for placement in self._placements:
            r = placement.rect
            side = placement.side
            horizontal = side in ("top", "bottom")
            pitch = r.width() / 0.72 if horizontal else r.height() / 0.72
            if pitch < metrics.height() * 0.95:
                continue                                    # too dense to label every pad
            text = str(placement.pad.die_pad)

            if upright.get(side):
                h = metrics.height()
                y = die.top() - gap - h if side == "top" else die.bottom() + gap
                box = QRectF(r.center().x() - pitch / 2, y, pitch, h)
                painter.drawText(box, Qt.AlignmentFlag.AlignCenter, text)
                continue

            # Sideways (dense top/bottom) or left/right: labels read away from the die.
            if side == "top":
                anchor, angle = QPointF(r.center().x(), die.top() - gap), -90
            elif side == "bottom":
                anchor, angle = QPointF(r.center().x(), die.bottom() + gap), 90
            elif side == "right":
                anchor, angle = QPointF(die.right() + gap, r.center().y()), 0
            else:
                anchor, angle = QPointF(die.left() - gap, r.center().y()), 0
            box = QRectF(0, -metrics.height() / 2, LABEL_MARGIN, metrics.height())
            align = Qt.AlignmentFlag.AlignLeft
            if side == "left":
                box.moveRight(0)
                align = Qt.AlignmentFlag.AlignRight
            painter.save()
            painter.translate(anchor)
            painter.rotate(angle)
            painter.drawText(box, align | Qt.AlignmentFlag.AlignVCenter, text)
            painter.restore()

    # ——— interaction ————————————————————————————————————————————————————————

    def mousePressEvent(self, event: QMouseEvent) -> None:
        die_pad = self.pad_at(event.position())
        if die_pad is not None:
            self.set_selected(die_pad)
            self.pad_clicked.emit(die_pad)

    def mouseMoveEvent(self, event: QMouseEvent) -> None:
        over_pad = self.pad_at(event.position()) is not None
        self.setCursor(Qt.CursorShape.PointingHandCursor if over_pad else Qt.CursorShape.ArrowCursor)

    def event(self, event) -> bool:
        if isinstance(event, QHelpEvent):
            die_pad = self.pad_at(QPointF(event.pos()))
            if die_pad is None:
                QToolTip.hideText()
            else:
                QToolTip.showText(event.globalPos(), self._tooltip(die_pad), self)
            return True
        return super().event(event)

    def _tooltip(self, die_pad: int) -> str:
        result = self._results.get(die_pad)
        if result is not None:
            return pad_summary(result)
        role = self._shape.role_of(die_pad) if self._shape else None
        return f"Die pad {die_pad}" + (f" · {role.value}" if role else "")


class DieLegend(QLabel):
    """One wrapping line naming the die drawing's colours: pad fill shows the
    result, pad outline shows the role."""

    def __init__(self, parent: QWidget | None = None) -> None:
        super().__init__(parent, wordWrap=True, textFormat=Qt.TextFormat.RichText,
                         alignment=Qt.AlignmentFlag.AlignCenter)
        self._update_text()

    def changeEvent(self, event: QEvent) -> None:
        if event.type() == QEvent.Type.PaletteChange:   # light/dark switch
            self._update_text()
        super().changeEvent(event)

    def _update_text(self) -> None:
        pal = self.palette()
        mid = pal.color(QPalette.ColorRole.Mid)
        fills = [("■", GOOD_COLOR, "Good"), ("■", OPEN_COLOR, "Open"),
                 ("■", UNTESTED_COLOR, "Not tested yet"), ("□", mid, "Not measured")]
        # Not the selection outline: its blue reads as Ground in a glyph, and a
        # selected pad is plain to see.
        outlines = [("□", POWER_COLOR, "Supply"), ("□", GND_COLOR, "Ground")]

        def entries(items) -> str:
            # Entries are kept whole (&nbsp; inside) and wrap between each other.
            return " &nbsp; ".join(f'<span style="color:{color.name()}">{glyph}</span>&nbsp;{label}'
                                   for glyph, color, label in items)

        divider = f' &nbsp;<span style="color:{mid.name()}">│</span>&nbsp; '
        self.setText(entries(fills) + divider + entries(outlines))

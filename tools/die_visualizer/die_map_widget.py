from PySide6.QtWidgets import QWidget, QHeaderView, QTableWidget, QTableWidgetItem, QSplitter
from PySide6.QtCore import Qt, QRect
from PySide6.QtGui import QPainter, QColor, QFont, QPen, QFontMetrics

from .die_shapes import DieShape
from .result_types import BondResult, PadRole, PadResult

COLOR_MAP = {
    (BondResult.GOOD, PadRole.IO): QColor("#4CAF50"),
    (BondResult.GOOD, PadRole.VDDIO): QColor("#4CAF50"),
    (BondResult.GOOD, PadRole.VDD_CORE): QColor("#4CAF50"),
    (BondResult.GOOD, PadRole.PWR_AUX): QColor("#4CAF50"),
    (BondResult.GOOD, PadRole.GND): QColor("#9E9E9E"),
    (BondResult.OPEN, PadRole.IO): QColor("#F44336"),
    (BondResult.OPEN, PadRole.VDDIO): QColor("#F44336"),
    (BondResult.OPEN, PadRole.VDD_CORE): QColor("#F44336"),
    (BondResult.OPEN, PadRole.PWR_AUX): QColor("#F44336"),
    (BondResult.OPEN, PadRole.GND): QColor("#F44336"),
    (BondResult.SHORT_GND, PadRole.IO): QColor("#F44336"),
    (BondResult.SHORT_GND, PadRole.VDDIO): QColor("#F44336"),
    (BondResult.SHORT_GND, PadRole.VDD_CORE): QColor("#F44336"),
    (BondResult.SHORT_GND, PadRole.PWR_AUX): QColor("#F44336"),
    (BondResult.SHORT_GND, PadRole.GND): QColor("#F44336"),
}

NEIGHBOUR_SHORT_COLOR = QColor("#FFEB3B")
UNTESTED_COLOR = QColor("#E0E0E0")
GND_BUS_ONLY_COLOR = QColor("#616161")
NC_COLOR = QColor("#424242")

MARGIN = 15
MIN_PAD_SIZE = 16
MAX_PAD_SIZE = 60
SPACING = 2
GAP = 3


def _pad_color(pad_info, result):
    if pad_info.role == PadRole.GND_BUS_ONLY:
        return GND_BUS_ONLY_COLOR
    if pad_info.role == PadRole.NC:
        return NC_COLOR
    if result is None:
        return UNTESTED_COLOR
    if result.prev_short or result.next_short:
        return NEIGHBOUR_SHORT_COLOR
    return COLOR_MAP.get((result.bond, pad_info.role), UNTESTED_COLOR)


def _bond_short(bond: BondResult) -> str:
    return {"GOOD": "OK", "OPEN": "OPEN", "SHORT_GND": "SHRT"}.get(bond.value, bond.value)


def _role_short(role: PadRole) -> str:
    return {
        "IO": "IO", "VDDIO": "VDD", "VDD_CORE": "VCORE",
        "PWR_AUX": "PWR", "GND": "GND", "GND_BUS_ONLY": "GBUS", "NC": "NC",
    }.get(role.value, role.value)


class DieMapWidget(QWidget):
    def __init__(self, shape: DieShape, results: dict | None = None, result_text: str = "", parent=None):
        super().__init__(parent)
        self.shape = shape
        self.results = results or {}
        self.result_text = result_text
        self._pad_rects = []
        self._cached_layout = None
        self.setMinimumSize(500, 500)

    def _compute_layout(self):
        w = self.width()
        h = self.height()
        R = self.shape.aspect_ratio
        nc = self.shape.north_count
        ec = self.shape.east_count
        sc = self.shape.south_count
        wc = self.shape.west_count

        # Strategy: size the die to fill available space maintaining aspect ratio,
        # then pick the largest pad_size where pad rows fit ALONG the die edges
        # (they don't need to span the full edge — they're centered on it).
        for ps in range(MAX_PAD_SIZE, MIN_PAD_SIZE - 1, -1):
            pad_room = ps + GAP
            avail_w = w - 2 * MARGIN - 2 * pad_room
            avail_h = h - 2 * MARGIN - 2 * pad_room
            if avail_w <= 0 or avail_h <= 0:
                continue

            # Die fills available space with aspect ratio
            if avail_w / avail_h > R:
                die_h = avail_h
                die_w = die_h * R
            else:
                die_w = avail_w
                die_h = die_w / R

            # Pad rows are centered along each edge; they just need to be shorter than the edge
            ns_row_w = max(nc, sc) * (ps + SPACING) - SPACING
            ew_col_h = max(ec, wc) * (ps + SPACING) - SPACING

            if ns_row_w <= die_w and ew_col_h <= die_h:
                die_x = MARGIN + pad_room + (avail_w - die_w) / 2
                die_y = MARGIN + pad_room + (avail_h - die_h) / 2
                return ps, die_x, die_y, die_w, die_h

        # Fallback: smallest pad size, die at aspect ratio
        ps = MIN_PAD_SIZE
        pad_room = ps + GAP
        avail_w = w - 2 * MARGIN - 2 * pad_room
        avail_h = h - 2 * MARGIN - 2 * pad_room
        avail_w = max(avail_w, 50)
        avail_h = max(avail_h, 50)
        if avail_w / avail_h > R:
            die_h = avail_h
            die_w = die_h * R
        else:
            die_w = avail_w
            die_h = die_w / R
        die_x = MARGIN + pad_room + (avail_w - die_w) / 2
        die_y = MARGIN + pad_room + (avail_h - die_h) / 2
        return ps, die_x, die_y, die_w, die_h

    def _compute_pad_rects(self):
        self._pad_rects = []
        pad_size, die_x, die_y, die_w, die_h = self._compute_layout()
        nc = self.shape.north_count
        ec = self.shape.east_count
        sc = self.shape.south_count
        wc = self.shape.west_count
        ring = self.shape.ring

        # Ring layout by DP order:
        #   North = ring[0:nc]        (DP 0–16)
        #   East  = ring[nc:nc+ec]    (DP 17–36) — renders on LEFT side
        #   South = ring[nc+ec:nc+ec+sc] (DP 37–53)
        #   West  = ring[nc+ec+sc:]   (DP 54–73) — renders on RIGHT side
        #
        # CCW from DP 0 (top-right corner):
        #   Top:    North, right-to-left
        #   Left:   East,  top-to-bottom
        #   Bottom: South, left-to-right
        #   Right:  West,  bottom-to-top
        north = ring[0:nc]
        left = ring[nc:nc + ec]
        south = ring[nc + ec:nc + ec + sc]
        right = ring[nc + ec + sc:]

        # Top (North): right-to-left — DP 0 at rightmost
        top_total_w = (nc - 1) * (pad_size + SPACING) + pad_size
        top_x0 = die_x + (die_w - top_total_w) / 2
        for i, pad in enumerate(north):
            x = top_x0 + top_total_w - pad_size - i * (pad_size + SPACING)
            y = die_y - pad_size - GAP
            self._pad_rects.append((pad, QRect(int(x), int(y), int(pad_size), int(pad_size))))

        # Left: top-to-bottom — DP 17 nearest top
        left_total_h = (ec - 1) * (pad_size + SPACING) + pad_size
        left_y0 = die_y + (die_h - left_total_h) / 2
        for i, pad in enumerate(left):
            x = die_x - pad_size - GAP
            y = left_y0 + i * (pad_size + SPACING)
            self._pad_rects.append((pad, QRect(int(x), int(y), int(pad_size), int(pad_size))))

        # Bottom (South): left-to-right — DP 37 at leftmost
        bot_total_w = (sc - 1) * (pad_size + SPACING) + pad_size
        bot_x0 = die_x + (die_w - bot_total_w) / 2
        for i, pad in enumerate(south):
            x = bot_x0 + i * (pad_size + SPACING)
            y = die_y + die_h + GAP
            self._pad_rects.append((pad, QRect(int(x), int(y), int(pad_size), int(pad_size))))

        # Right: bottom-to-top — DP 73 at bottom
        right_total_h = (wc - 1) * (pad_size + SPACING) + pad_size
        right_y0 = die_y + (die_h - right_total_h) / 2
        for i, pad in enumerate(right):
            x = die_x + die_w + GAP
            y = right_y0 + right_total_h - pad_size - i * (pad_size + SPACING)
            self._pad_rects.append((pad, QRect(int(x), int(y), int(pad_size), int(pad_size))))

    def resizeEvent(self, event):
        super().resizeEvent(event)
        self._compute_pad_rects()

    def paintEvent(self, event):
        if not self._pad_rects:
            self._compute_pad_rects()

        painter = QPainter(self)
        painter.setRenderHint(QPainter.Antialiasing)

        pad_size, die_x, die_y, die_w, die_h = self._compute_layout()

        # Draw die rectangle
        painter.setPen(QPen(QColor("#555555"), 2))
        painter.setBrush(QColor("#1a1a2e"))
        painter.drawRoundedRect(int(die_x), int(die_y), int(die_w), int(die_h), 6, 6)

        # Pin 1 indicator dot (top-right corner of die)
        dot_r = max(4, int(pad_size * 0.18))
        dot_cx = int(die_x + die_w - dot_r - 6)
        dot_cy = int(die_y + dot_r + 6)
        painter.setPen(Qt.PenStyle.NoPen)
        painter.setBrush(QColor("#ffffff"))
        painter.drawEllipse(dot_cx - dot_r, dot_cy - dot_r, dot_r * 2, dot_r * 2)

        # Draw die label
        painter.setPen(QColor("#aaaaaa"))
        name_font = QFont("Menlo, Consolas, Courier New, monospace", max(10, int(pad_size * 0.45)))
        name_font.setBold(True)
        painter.setFont(name_font)
        painter.drawText(QRect(int(die_x), int(die_y), int(die_w), int(die_h)),
                         Qt.AlignHCenter | Qt.AlignTop, self.shape.name)

        if self.result_text:
            is_fail = "FAIL" in self.result_text
            result_color = QColor("#F44336") if is_fail else QColor("#4CAF50")
            bg_color = QColor("#3a1a1a") if is_fail else QColor("#1a3a1a")

            status_font = QFont("Menlo, Consolas, Courier New, monospace", max(18, int(pad_size * 1.2)))
            status_font.setBold(True)
            fm = QFontMetrics(status_font)
            sw = fm.horizontalAdvance(self.result_text)
            sh = fm.height()
            box_w = sw + 24
            box_h = sh + 12
            box_x = int(die_x + (die_w - box_w) / 2)
            box_y = int(die_y + (die_h - box_h) / 2)

            painter.setPen(QPen(result_color, 2))
            painter.setBrush(bg_color)
            painter.drawRoundedRect(box_x, box_y, box_w, box_h, 8, 8)

            painter.setPen(result_color)
            painter.setFont(status_font)
            painter.drawText(QRect(box_x, box_y, box_w, box_h), Qt.AlignCenter, self.result_text)

        # Draw pads
        pad_font = QFont("Menlo, Consolas, Courier New, monospace", max(8, int(pad_size * 0.38)))
        painter.setFont(pad_font)
        fm = QFontMetrics(pad_font)

        for pad_info, rect in self._pad_rects:
            result = self.results.get(pad_info.die_pad)
            color = _pad_color(pad_info, result)

            painter.setPen(QPen(QColor("#222222"), 1))
            painter.setBrush(color)
            painter.drawRoundedRect(rect, 3, 3)

            label = str(pad_info.die_pad)
            tx = rect.x() + (rect.width() - fm.horizontalAdvance(label)) // 2
            ty = rect.y() + (rect.height() + fm.ascent() - fm.descent()) // 2
            painter.setPen(QColor("#ffffff") if color.lightness() < 128 else QColor("#000000"))
            painter.drawText(tx, ty, label)


def build_results_table(shape: DieShape, results: dict) -> QTableWidget:
    columns = ["DP", "Role", "Bond", "Sense V", "Prev V", "Next V", "Flags"]
    table = QTableWidget(len(shape.ring), len(columns))
    table.setHorizontalHeaderLabels(columns)
    table.setEditTriggers(QTableWidget.EditTrigger.NoEditTriggers)
    table.setSelectionBehavior(QTableWidget.SelectionBehavior.SelectRows)
    table.setAlternatingRowColors(True)

    for i, pad_info in enumerate(shape.ring):
        result = results.get(pad_info.die_pad)
        if pad_info.role == PadRole.GND_BUS_ONLY:
            bond_str = "—"
            sv = pv = nv = "—"
            flags = "bus-only"
        elif result is None:
            bond_str = "—"
            sv = pv = nv = "—"
            flags = "untested"
        else:
            bond_str = _bond_short(result.bond)
            sv = f"{result.sense_v:.3f}"
            pv = f"{result.prev_v:.3f}"
            nv = f"{result.next_v:.3f}"
            parts = []
            if result.prev_short:
                parts.append("pshort")
            if result.next_short:
                parts.append("nshort")
            flags = " ".join(parts)

        row = [str(pad_info.die_pad), _role_short(pad_info.role), bond_str, sv, pv, nv, flags]
        for c, val in enumerate(row):
            item = QTableWidgetItem(val)
            item.setTextAlignment(Qt.AlignCenter)
            if c == 2 and val not in ("—", ""):
                color = _pad_color(pad_info, result)
                item.setBackground(color)
                item.setForeground(QColor("#ffffff") if color.lightness() < 128 else QColor("#000000"))
            table.setItem(i, c, item)

    header = table.horizontalHeader()
    header.setSectionResizeMode(QHeaderView.ResizeMode.ResizeToContents)
    table.verticalHeader().setVisible(False)

    return table


def create_main_layout(shape: DieShape, results_by_die_pad: dict, result) -> QSplitter:
    result_text = f"{result.good_count}/{result.tested_count} {result.outcome}"
    die_map = DieMapWidget(shape=shape, results=results_by_die_pad, result_text=result_text)
    table = build_results_table(shape, results_by_die_pad)

    splitter = QSplitter(Qt.Orientation.Horizontal)
    splitter.addWidget(die_map)
    splitter.addWidget(table)
    splitter.setStretchFactor(0, 3)
    splitter.setStretchFactor(1, 2)

    return splitter
import math
import os

import pyqtgraph as pg
import pytest

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")

from PySide6.QtCore import QPointF, Qt  # noqa: E402
from PySide6.QtWidgets import QApplication  # noqa: E402

from bondtest72_gui import protocol as p  # noqa: E402
from bondtest72_gui.assembler import ResultAssembler  # noqa: E402
from bondtest72_gui.charts import IoChart, IoIvChart, PowerChart  # noqa: E402
from bondtest72_gui.pad_table import PadGroup, PadTableModel  # noqa: E402


@pytest.fixture
def result(trace_lines):
    QApplication.instance() or QApplication([])
    # Keep the view toggle's QSettings writes out of the real app's settings.
    QApplication.setOrganizationName("BondTest72-tests")
    QApplication.setApplicationName("BondTest72-tests")
    assembler = ResultAssembler()
    return next(r for line in trace_lines if (r := assembler.feed(p.parse_line(line))))


def test_table_has_one_column_per_reading_headed_by_level(result):
    io, power = PadTableModel(PadGroup.IO), PadTableModel(PadGroup.POWER)
    io.set_result(result)
    power.set_result(result)

    def headers(model):
        return [model.headerData(c, Qt.Orientation.Horizontal) for c in range(model.columnCount())]

    assert headers(io)[4:] == ["Rev 12 µA", "Rev 120 µA", "Rev 1325 µA"]
    assert headers(power)[4:] == ["Rev 156 µs", "Rev 312 µs", "Rev 1.25 ms", "Rev 10 ms", "Rev 20 ms"]
    assert io.rowCount() + power.rowCount() == 64


def test_charts_take_a_result_and_a_selection(result):
    for chart in (IoChart(), IoIvChart(), PowerChart()):
        chart.set_result(result)
        chart.set_selected(17)
        chart.set_result(None)


def test_selecting_a_pad_from_outside_moves_the_whole_row_selection(result):
    from bondtest72_gui.results_panel import PadGroupPanel

    panel = PadGroupPanel(PadGroup.IO)
    panel.set_result(result)
    reported = []
    panel.pad_selected.connect(reported.append)

    panel.table.selectRow(5)                           # user picks a row in the table
    first = reported[-1]
    assert panel.select_pad(result.pads[0].dp)         # then a pad on the die

    rows = {i.row() for i in panel.table.selectionModel().selectedIndexes()}
    assert rows == {0}                                 # old row deselected, new row fully selected
    assert len(panel.table.selectionModel().selectedIndexes()) == panel.model.columnCount()
    assert reported == [first]                         # not echoed back as a user choice


def test_clicking_overlapping_points_selects_the_first_pad(result):
    import numpy as np

    chart = IoChart()
    chart.set_result(result)
    clicked = []
    chart.pad_clicked.connect(clicked.append)
    scatter = next(item for item in chart.getPlotItem().items if hasattr(item, "sigClicked"))
    spots = np.array(scatter.points()[:2], dtype=object)      # pyqtgraph sends an array
    scatter.sigClicked.emit(scatter, spots, None)
    assert clicked == [spots[0].data()[0]]


def test_io_panel_toggles_between_graph_types(result):
    from bondtest72_gui.results_panel import PadGroupPanel

    io, power = PadGroupPanel(PadGroup.IO), PadGroupPanel(PadGroup.POWER)
    assert len(io._view_buttons.buttons()) == 2 and power._view_buttons.buttons() == []
    io._view_buttons.button(1).click()
    assert io._stack.currentWidget() is io.charts[1]
    io._view_buttons.button(0).click()
    assert io._stack.currentWidget() is io.charts[0]


def test_io_charts_show_at_least_one_volt_and_stretch_for_higher_readings(result):
    import dataclasses

    low = dataclasses.replace(result, pads=[dataclasses.replace(p, vr=[0.5, 0.6, 0.8], rr=None)
                                            for p in result.pads if not p.is_cap])
    for chart in (IoChart(), IoIvChart()):
        chart.set_result(low)
        assert chart.getPlotItem().viewRange()[1] == [0, 1.0]
        chart.set_result(result)                           # empty-substrate trace: all pads near 3.3 V
        assert chart.getPlotItem().viewRange()[1][1] > 3.0


class _Wheel:
    """Just enough of a pyqtgraph wheel event for AxisItem/ViewBox.wheelEvent."""

    def __init__(self, scene_pos, delta=120):
        self._scene_pos, self._delta = scene_pos, delta

    def scenePos(self):
        return self._scene_pos

    def delta(self):
        return self._delta

    def accept(self):
        pass

    def ignore(self):
        pass


class _Drag(_Wheel):
    """Just enough of a pyqtgraph left-button drag event, moved by `dy` pixels."""

    def __init__(self, scene_pos, dy):
        super().__init__(scene_pos)
        self._dy = dy

    def buttonDownScenePos(self):
        return self._scene_pos

    def button(self):
        return Qt.MouseButton.LeftButton

    def pos(self):
        return pg.Point(0, self._dy)

    def lastPos(self):
        return pg.Point(0, 0)


def test_scrolling_or_dragging_an_axis_moves_only_that_axis_and_A_restores_the_view(result):
    chart = IoChart()                       # scrolling over this plot does nothing
    chart.resize(600, 400)
    chart.show()
    chart.set_result(result)
    QApplication.processEvents()
    item = chart.getPlotItem()
    home = [list(r) for r in item.viewRange()]

    item.vb.wheelEvent(_Wheel(item.vb.sceneBoundingRect().center()))
    assert [list(r) for r in item.viewRange()] == home

    left = item.getAxis("left")
    labels = left.sceneBoundingRect()       # spans the plot too (it draws the grid)
    left.wheelEvent(_Wheel(QPointF(labels.left() + 10, labels.center().y())))
    (x, y) = item.viewRange()
    assert x == home[0] and y != home[1]

    left.mouseDragEvent(_Drag(QPointF(labels.left() + 10, labels.center().y()), dy=-40))
    (x, y_dragged) = item.viewRange()
    assert x == home[0] and y_dragged[0] < y[0] and math.isclose(y_dragged[1] - y_dragged[0], y[1] - y[0])

    item.mouseHovering = True
    item.updateButtons()
    assert item.autoBtn.isVisible()
    item.autoBtn.clicked.emit(item.autoBtn)
    assert [list(r) for r in item.viewRange()] == home
    assert not item.autoBtn.isVisible()     # back home: nothing to reset


def test_plot_fills_its_widget_so_a_touchpad_cannot_scroll_it(result):
    import qdarktheme

    qdarktheme.setup_theme("dark")          # the theme is what adds a frame
    try:
        chart = IoChart()
        chart.resize(600, 400)
        chart.show()
        QApplication.processEvents()
        assert chart.plot.viewport().size() == chart.plot.size()
        assert chart.plot.horizontalScrollBar().maximum() == 0
        assert chart.plot.verticalScrollBar().maximum() == 0
    finally:
        QApplication.instance().setStyleSheet("")

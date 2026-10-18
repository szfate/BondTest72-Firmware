"""One pad group's results: the sortable table with a graph underneath."""

from __future__ import annotations

from PySide6.QtCore import QItemSelectionModel, QModelIndex, QSettings, QSortFilterProxyModel, Qt, Signal
from PySide6.QtWidgets import (
    QAbstractItemView, QButtonGroup, QHBoxLayout, QHeaderView, QSplitter, QStackedWidget, QTableView,
    QToolButton, QVBoxLayout, QWidget,
)

from .charts import IoChart, IoIvChart, PowerChart
from .models import TestResult
from .pad_table import SORT_ROLE, PadGroup, PadTableModel

_SEGMENT_STYLE = """
QToolButton { border: 1px solid palette(mid); border-radius: 0; padding: 3px 12px;
              background: transparent; color: palette(window-text); }
QToolButton:hover:!checked { background: palette(alternate-base); }
QToolButton:checked { background: palette(highlight); border-color: palette(highlight);
                      color: palette(highlighted-text); }
QToolButton[segment="first"] { border-top-left-radius: 11px; border-bottom-left-radius: 11px; }
QToolButton[segment="last"] { border-top-right-radius: 11px; border-bottom-right-radius: 11px; }
QToolButton[segment="middle"], QToolButton[segment="last"] { border-left: none; }
"""


class PadGroupPanel(QWidget):
    pad_selected = Signal(int)          # die pad, chosen in the table or the graph

    def __init__(self, group: PadGroup, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.group = group
        self.model = PadTableModel(group)
        self._proxy = QSortFilterProxyModel(self)
        self._proxy.setSourceModel(self.model)
        self._proxy.setSortRole(SORT_ROLE)

        self.table = QTableView()
        self.table.setModel(self._proxy)
        self.table.setSortingEnabled(True)
        self.table.sortByColumn(-1, Qt.SortOrder.AscendingOrder)   # tester order until a header is clicked
        self.table.setSelectionBehavior(QAbstractItemView.SelectionBehavior.SelectRows)
        self.table.setSelectionMode(QAbstractItemView.SelectionMode.SingleSelection)
        self.table.setEditTriggers(QAbstractItemView.EditTrigger.NoEditTriggers)
        self.table.verticalHeader().setVisible(False)
        self.table.verticalHeader().setDefaultSectionSize(22)
        self.table.horizontalHeader().setSectionResizeMode(QHeaderView.ResizeMode.ResizeToContents)
        # Columns keep their content width; spare room stays empty after the last one.
        self.table.horizontalHeader().setStretchLastSection(False)

        # IO pads get two graph types, swapped with a toggle: per pad, and all
        # pads' IV curves on one graph. The choice is remembered across sessions.
        if group is PadGroup.IO:
            views = [("By pad", IoChart()), ("IV curves", IoIvChart())]
        else:
            views = [("Charging curves", PowerChart())]
        self.charts = [chart for _, chart in views]
        self._stack = QStackedWidget()
        for _, chart in views:
            self._stack.addWidget(chart)
            chart.pad_clicked.connect(self._chart_clicked)

        chart_area = QWidget()
        chart_layout = QVBoxLayout(chart_area)
        chart_layout.setContentsMargins(0, 4, 0, 0)
        chart_layout.setSpacing(2)
        self._view_buttons = QButtonGroup(self)
        if len(views) > 1:
            # A segmented control: the buttons share one rounded outline and the
            # chosen one is filled with the accent colour.
            toggle = QHBoxLayout()
            toggle.setSpacing(0)
            toggle.addStretch()
            for i, (name, _) in enumerate(views):
                button = QToolButton(text=name, checkable=True, autoRaise=False)
                button.setProperty("segment", "first" if i == 0 else "last" if i == len(views) - 1 else "middle")
                button.setStyleSheet(_SEGMENT_STYLE)
                self._view_buttons.addButton(button, i)
                toggle.addWidget(button)
            toggle.addStretch()
            chart_layout.addLayout(toggle)
            self._view_buttons.idClicked.connect(self.show_view)
        chart_layout.addWidget(self._stack, 1)
        saved = QSettings().value(self._view_setting, 0, type=int)
        self.show_view(saved if 0 <= saved < len(views) else 0)

        # True while select_pad() moves the selection, so the change isn't
        # reported back as the user's choice. (Blocking the selection model's
        # signals instead would also stop the table from repainting.)
        self._syncing = False

        splitter = QSplitter(Qt.Orientation.Vertical)
        splitter.addWidget(self.table)
        splitter.addWidget(chart_area)
        splitter.setStretchFactor(0, 1)
        splitter.setStretchFactor(1, 1)
        splitter.setSizes([300, 300])
        layout = QVBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.addWidget(splitter)

        self.table.selectionModel().currentRowChanged.connect(self._row_changed)

    @property
    def _view_setting(self) -> str:
        return f"charts/{self.group.name.lower()}_view"

    def show_view(self, index: int) -> None:
        """Show graph type `index` (see the toggle) and remember the choice."""
        self._stack.setCurrentIndex(index)
        button = self._view_buttons.button(index)
        if button is not None:
            button.setChecked(True)
        QSettings().setValue(self._view_setting, index)

    @property
    def pad_count(self) -> int:
        return self.model.rowCount()

    def set_result(self, result: TestResult | None) -> None:
        self.model.set_result(result)
        for chart in self.charts:
            chart.set_result(result)

    def select_pad(self, die_pad: int | None) -> bool:
        """Select a pad in the table and graph. False if this group doesn't have it."""
        row = self.model.row_of(die_pad) if die_pad is not None else None
        for chart in self.charts:
            chart.set_selected(die_pad if row is not None else None)
        selection = self.table.selectionModel()
        self._syncing = True
        try:
            if row is None:
                selection.clear()
                return False
            index = self._proxy.mapFromSource(self.model.index(row, 0))
            selection.setCurrentIndex(index, QItemSelectionModel.SelectionFlag.ClearAndSelect
                                      | QItemSelectionModel.SelectionFlag.Rows)
            self.table.scrollTo(index)
            return True
        finally:
            self._syncing = False

    def _row_changed(self, current: QModelIndex, _previous: QModelIndex) -> None:
        if self._syncing:
            return
        die_pad = self.model.die_pad_at(self._proxy.mapToSource(current).row())
        if die_pad is not None:
            for chart in self.charts:
                chart.set_selected(die_pad)
            self.pad_selected.emit(die_pad)

    def _chart_clicked(self, die_pad: int) -> None:
        self.select_pad(die_pad)
        self.pad_selected.emit(die_pad)

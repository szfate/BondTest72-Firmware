"""Smoke tests for the main window, rendered offscreen."""

import os

import pytest

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")

from PySide6.QtCore import QCoreApplication, QSettings  # noqa: E402
from PySide6.QtWidgets import QApplication  # noqa: E402

from bondtest72_gui import main_window as mw  # noqa: E402
from bondtest72_gui.assembler import ResultAssembler  # noqa: E402
from bondtest72_gui import protocol as p  # noqa: E402


@pytest.fixture
def window(monkeypatch, tmp_path):
    app = QApplication.instance() or QApplication([])
    QCoreApplication.setOrganizationName("BondTest72-tests")
    QCoreApplication.setApplicationName("BondTest72-tests")
    QSettings().clear()
    QSettings().setValue("results_folder", str(tmp_path))
    # Never find (and auto-connect to) a real tester on the bench.
    monkeypatch.setattr(mw, "find_tester_ports", lambda: [])
    w = mw.MainWindow()
    w.show()
    app.processEvents()
    yield w
    w.close()
    QSettings().clear()


def trace_result(trace_lines):
    assembler = ResultAssembler()
    results = [r for line in trace_lines if (r := assembler.feed(p.parse_line(line)))]
    return results[0]


def test_starts_disconnected(window):
    assert not window._run_button.isEnabled()
    assert window._connect_button.text() == "Connect"


def test_shows_the_default_pad_maps_die_until_a_tester_reports_one(window):
    assert window._die_view._shape is mw.shape_for_padmap(mw.DEFAULT_PADMAP) is not None


def test_live_result_is_shown_recorded_and_tagged(window, trace_lines, tmp_path):
    window._sample_edit.setText("PANEL-7")
    window._on_result(trace_result(trace_lines), True)

    assert window._banner.text().startswith("FAIL  0/64")
    assert [panel.pad_count for panel in window._panels] == [56, 8]
    [saved] = tmp_path.glob("*.jsonl")
    assert '"panel_id":"PANEL-7"' in saved.read_text()


def test_replayed_result_is_not_recorded(window, trace_lines, tmp_path):
    window._on_result(trace_result(trace_lines), False)
    assert window._history_list.count() == 1
    assert list(tmp_path.glob("*.jsonl")) == []


def test_changing_the_pad_map_clears_the_result_and_redraws_the_die(window, trace_lines):
    feed = window._conn._on_line
    for line in ["ADAPTER aid=0000AA ahw=2 pm=2,3 ins=1 tests=1 dut=1 active=2 state=READY",
                 "PADMAP id=2 pads=64 default=1 active=1 name=1x1 Mezzanine70 v2",
                 "PADMAP id=3 pads=72 default=0 active=0 name=1x0p5",
                 "OK LIST_PADMAPS"]:
        feed(line)
    window._on_result(trace_result(trace_lines), False)
    assert window._shown is not None

    feed("ADAPTER aid=0000AA ahw=2 pm=2,3 ins=1 tests=1 dut=1 active=2 state=PASS")    # same map: result stays
    assert window._shown is not None

    feed("ADAPTER aid=0000AA ahw=2 pm=2,3 ins=1 tests=1 dut=1 active=3 state=READY")   # after SET_PADMAP id=3
    assert window._shown is None
    assert [panel.pad_count for panel in window._panels] == [0, 0]
    assert window._die_view._shape is mw.shape_for_padmap(3)
    assert window._history_list.count() == 1 and window._history_list.currentRow() == -1


def test_toolbar_button_hides_the_bottom_panel_and_the_choice_is_remembered(window):
    assert not window._dock.isHidden() and window._panel_button.isChecked()
    window._panel_button.click()
    assert window._dock.isHidden() and not window._panel_button.isChecked()

    window.close()
    reopened = mw.MainWindow()
    assert reopened._dock.isHidden() and not reopened._panel_action.isChecked()
    reopened._panel_action.trigger()                  # View menu / Ctrl+J
    assert not reopened._dock.isHidden() and reopened._panel_button.isChecked()
    reopened.close()


def test_app_icon_loads(window):
    from PySide6.QtGui import QIcon

    from bondtest72_gui.app import ICON
    assert not QIcon(str(ICON)).pixmap(64, 64).isNull()

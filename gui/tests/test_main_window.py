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

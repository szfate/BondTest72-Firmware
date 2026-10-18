import queue

from bondtest72_gui.tester_serial import TesterSerial


def open_loopback():
    """A connection on pyserial's loopback port: everything sent comes straight back."""
    lines: queue.Queue[str] = queue.Queue()
    closed: queue.Queue[Exception | None] = queue.Queue()
    link = TesterSerial("loop://", on_line=lines.put, on_closed=closed.put)
    return link, lines, closed


def test_lines_are_delivered_without_newlines():
    link, lines, _ = open_loopback()
    try:
        link.send("HELLO")                   # newline added automatically
        link.send("GET_ADAPTER\r\n")
        assert lines.get(timeout=1) == "HELLO"
        assert lines.get(timeout=1) == "GET_ADAPTER"
    finally:
        link.close()


def test_blank_lines_are_skipped():
    link, lines, _ = open_loopback()
    try:
        link.send("\n")
        link.send("OK LIST_PADMAPS")
        assert lines.get(timeout=1) == "OK LIST_PADMAPS"
    finally:
        link.close()


def test_close_is_reported_once():
    link, _, closed = open_loopback()
    link.close()
    link.close()
    assert closed.get(timeout=1) is None
    assert closed.empty()

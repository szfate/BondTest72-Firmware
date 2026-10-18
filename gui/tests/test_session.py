from datetime import datetime

from bondtest72_gui import protocol as p
from bondtest72_gui.session import TesterSession

ADAPTER_READY = ("ADAPTER aid=00000000000000AA ahw=2 pm=2,5 active=2 lifespan=9999 "
                 "mfg_date=20261015 ins=1 tests=4 eol=0 dut=1 state=READY")


def feed_all(session, lines):
    return [session.feed(line) for line in lines]


def test_startup_queries_identity_adapter_and_padmaps():
    assert TesterSession().startup_commands() == [p.cmd_hello(), p.cmd_get_adapter(), p.cmd_list_padmaps()]


def test_adapter_reply_sets_state_and_active_padmap():
    s = TesterSession()
    s.feed(ADAPTER_READY)
    assert s.state == "READY" and s.active_padmap == 2
    assert s.can_run


def test_padmap_list_is_replaced_only_when_the_reply_completes():
    s = TesterSession()
    feed_all(s, ["PADMAP id=1 pads=63 default=0 active=0 name=old", "OK LIST_PADMAPS"])
    s.feed("PADMAP id=2 pads=64 default=1 active=1 name=1x1 Mezzanine70 v2")
    assert [m.id for m in s.padmaps] == [1]              # second reply still arriving
    s.feed("OK LIST_PADMAPS")
    assert [m.id for m in s.padmaps] == [2]
    assert s.padmap_name(2) == "1x1 Mezzanine70 v2"


def test_events_trigger_a_state_refresh():
    s = TesterSession()
    assert s.feed("EVENT DUT_INSERTED").send == [p.cmd_get_adapter()]
    assert s.feed("EVENT ADAPTER_DETECTED aid=AA ahw=2 pm=2 active=2").send == [
        p.cmd_get_adapter(), p.cmd_list_padmaps()]
    assert s.feed("OK SET_PADMAP id=3").send == [p.cmd_get_adapter(), p.cmd_list_padmaps()]


def test_adapter_removed_clears_adapter_and_padmaps():
    s = TesterSession()
    feed_all(s, [ADAPTER_READY, "PADMAP id=2 pads=64 default=1 active=1 name=x", "OK LIST_PADMAPS",
                 "EVENT ADAPTER_REMOVED"])
    assert s.adapter is None and s.padmaps == [] and s.state == "NO_ADAPTER"
    assert not s.can_run and not s.can_set_padmap


def test_no_adapter_error_clears_adapter():
    s = TesterSession()
    feed_all(s, [ADAPTER_READY, "ERROR code=1 msg=NO_ADAPTER"])
    assert s.adapter is None and s.last_error == p.Error(1, "NO_ADAPTER")


def test_real_trace_gives_one_live_result(trace_lines):
    s = TesterSession(clock=lambda: datetime(2026, 10, 18, 12, 0))
    updates = feed_all(s, trace_lines)
    finished = [u for u in updates if u.result is not None]
    assert len(finished) == 1 and finished[0].live
    assert finished[0].send == [p.cmd_get_adapter()]
    assert s.state == "FAIL"
    assert s.hello is not None and s.hello.uid == "0000000000000001"


def test_get_results_replay_is_not_live(trace_lines):
    s = TesterSession()
    feed_all(s, trace_lines)
    replay = [line for line in trace_lines if line.startswith(("SLOT", "PAD ", "SUMMARY"))]
    finished = [u for u in feed_all(s, replay) if u.result is not None]
    assert len(finished) == 1 and not finished[0].live
    assert s.state == "FAIL"                              # a replay doesn't change state


def test_v1_firmware_without_state_falls_back_to_dut_flag():
    s = TesterSession()
    s.feed("ADAPTER aid=00 ahw=1 pm=2 lifespan=100 mfg_date=20260601 ins=3 tests=2 eol=0 dut=1")
    assert s.state is None and s.can_run

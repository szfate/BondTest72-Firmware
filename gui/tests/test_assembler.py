from datetime import datetime

from bondtest72_gui import protocol as p
from bondtest72_gui.assembler import ResultAssembler
from bondtest72_gui.models import BondResult, PadRole

FIXED_TIME = datetime(2026, 10, 15, 12, 0, 0)


def feed_all(assembler, lines):
    """Feed lines through the parser; return every finished result."""
    results = []
    for line in lines:
        result = assembler.feed(p.parse_line(line))
        if result is not None:
            results.append(result)
    return results


def test_real_trace_gives_one_complete_result(trace_lines):
    [result] = feed_all(ResultAssembler(clock=lambda: FIXED_TIME), trace_lines)

    assert result.ts == FIXED_TIME
    assert result.outcome == "FAIL" and result.good == 0 and result.tested == 64
    assert len(result.pads) == 64
    assert all(pad.result is BondResult.OPEN for pad in result.pads)

    ctx = result.context
    assert ctx.pm == 2
    assert ctx.pm_name == "1x1 Mezzanine70 v2"            # from the PADMAP list
    assert ctx.tester_uid == "0000000000000001"            # from HELLO
    assert ctx.current_list_ua == [12.0, 120.0, 1325.0]
    assert ctx.cap_time_list_us == [156, 312, 1250, 10000, 20000]


def test_pad_roles_come_from_type_field(trace_lines):
    [result] = feed_all(ResultAssembler(), trace_lines)
    roles = [pad.role for pad in result.pads]
    assert roles.count(PadRole.IO) == 56
    assert {pad.dp for pad in result.pads if pad.role.is_power} == {17, 25, 34, 36, 57, 63, 71, 73}
    # Every power pad is a charging-curve measurement.
    assert all(pad.is_cap for pad in result.pads if pad.role.is_power)


def test_v1_firmware_roles_fall_back_to_die_shape():
    lines = [
        "EVENT TEST_START aid=AB ahw=2 ins=1 tests=1 pm=2 current_list_ua=12,120,1325",
        "SLOT slot=0 present=1 tested=1",
        "PAD slot=0 apin=9 dp=17 method=CAP_REV result=GOOD vrs=0.3,0.4,0.5,0.6,0.6",
        "PAD slot=0 apin=63 dp=0 method=STD_REV result=GOOD rr=1,2,3 vr=0.1,0.2,0.3",
        "SUMMARY outcome=PASS good=2 tested=2",
    ]
    [result] = feed_all(ResultAssembler(), lines)
    assert result.context.pm == 2                         # no active= → first pm= entry
    assert [pad.role for pad in result.pads] == [PadRole.VDDIO, PadRole.IO]


def test_get_results_replay_reuses_last_test_settings(trace_lines):
    assembler = ResultAssembler()
    [first] = feed_all(assembler, trace_lines)

    # GET_RESULTS resends SLOT/PAD/SUMMARY without a TEST_START.
    replay = [line for line in trace_lines if line.startswith(("SLOT", "PAD", "SUMMARY"))]
    [again] = feed_all(assembler, replay)

    assert again.pads == first.pads
    assert again.context == first.context


def test_pad_lines_outside_a_result_are_ignored():
    assembler = ResultAssembler()
    assert assembler.feed(p.parse_line(
        "PAD slot=0 apin=63 dp=0 method=STD_REV result=GOOD rr=1,2,3 vr=0.1,0.2,0.3")) is None
    assert assembler.feed(p.parse_line("SUMMARY outcome=PASS good=1 tested=1")) is None


def test_reset_drops_a_partial_result(trace_lines):
    assembler = ResultAssembler()
    cut = trace_lines.index(next(l for l in trace_lines if l.startswith("SUMMARY")))
    feed_all(assembler, trace_lines[:cut])     # everything but SUMMARY
    assembler.reset()                          # e.g. connection lost
    assert assembler.feed(p.parse_line(trace_lines[cut])) is None

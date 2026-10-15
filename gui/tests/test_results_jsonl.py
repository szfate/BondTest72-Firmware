import json
from datetime import date

from bondtest72_gui import protocol as p
from bondtest72_gui import results_jsonl as rj
from bondtest72_gui.assembler import ResultAssembler

from conftest import DATA


def assembled(trace_lines):
    assembler = ResultAssembler()
    for line in trace_lines:
        result = assembler.feed(p.parse_line(line))
        if result is not None:
            return result
    raise AssertionError("trace has no complete result")


def test_round_trip_preserves_the_result(trace_lines):
    result = assembled(trace_lines)
    assert rj.from_record(json.loads(rj.to_line(result))) == result


def test_factory_records_load_and_rewrite_identically():
    """Files written by the factory tool must load here, and re-saving a record
    must reproduce it exactly — same keys, same order, same values."""
    lines = (DATA / "factory_format.jsonl").read_text().splitlines()
    for line in (lines[0], lines[2]):
        assert rj.to_line(rj.from_record(json.loads(line))) == line


def test_load_skips_damaged_lines():
    results = rj.load(DATA / "factory_format.jsonl")
    assert [r.outcome for r in results] == ["PASS", "FAIL"]
    assert results[0].panel_id == "EXAMPLE-PANEL-01"
    assert results[0].context.pm == 6
    assert results[1].context is None and results[1].reason == "DUT_REMOVED"


def test_unmeasured_groups_are_left_out(trace_lines):
    record = rj.to_record(assembled(trace_lines))
    std = next(pad for pad in record["pads"] if pad["method"] == "STD_REV")
    cap = next(pad for pad in record["pads"] if pad["method"] == "CAP_REV")
    assert set(std) == {"apin", "dp", "method", "result", "role", "rr", "vr"}
    assert set(cap) == {"apin", "dp", "method", "result", "role", "vrs"}


def test_pad_map_name_is_recorded(trace_lines):
    record = rj.to_record(assembled(trace_lines))
    assert record["context"]["pmName"] == "1x1 Mezzanine70 v2"


def test_append_creates_daily_file(tmp_path, trace_lines):
    path = rj.daily_file(tmp_path / "results", date(2026, 10, 15))
    assert path.name == "2026-10-15.jsonl"

    result = assembled(trace_lines)
    rj.append(path, result)
    rj.append(path, result)
    assert len(rj.load(path)) == 2

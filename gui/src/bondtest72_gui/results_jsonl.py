"""Results files: one JSON object per line (JSONL), one line per test.

The record layout is documented in docs/RESULTS_JSONL.md and matches the
factory tool's, so results files from either tool can be read by both:

    {"ts": ..., "outcome": "PASS", "good": 64, "tested": 64, "reason": null,
     "slot": 0, "pads": [...], "img": null, "panel_id": null,
     "context": {"aid": ..., "pm": 2, "currentListUa": [...], ...}}

Files are append-only and usually split per day (see daily_file()).
Unreadable lines are skipped on load rather than failing the whole file.
"""

from __future__ import annotations

import json
from datetime import date, datetime
from pathlib import Path
from typing import Any

from .models import BondResult, PadResult, PadRole, TestContext, TestResult

_MEASUREMENT_KEYS = ("rf", "rr", "vf", "vr", "vfs", "vrs")


# ——— writing ————————————————————————————————————————————————————————————————

def to_record(result: TestResult) -> dict[str, Any]:
    """The JSON object for one result. Key order follows the factory tool."""
    record: dict[str, Any] = {
        "ts": result.ts.isoformat(timespec="milliseconds"),
        "outcome": result.outcome,
        "good": result.good,
        "tested": result.tested,
        "reason": result.reason,
        "slot": result.slot,
        "pads": [_pad_record(pad) for pad in result.pads],
        "img": result.img,
        "panel_id": result.panel_id,
    }
    if result.thumb is not None:
        record["thumb"] = result.thumb
    if result.context is not None:
        record["context"] = _context_record(result.context)
    return record


def to_line(result: TestResult) -> str:
    """One compact JSONL line, without the trailing newline."""
    return json.dumps(to_record(result), separators=(",", ":"), ensure_ascii=False)


def append(path: Path, result: TestResult) -> None:
    """Append a result to a JSONL file, creating the file and folders if needed."""
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("a", encoding="utf-8") as f:
        f.write(to_line(result) + "\n")


def daily_file(folder: Path, day: date | None = None) -> Path:
    """The results file for a given day: <folder>/YYYY-MM-DD.jsonl."""
    return folder / f"{(day or date.today()).isoformat()}.jsonl"


def _pad_record(pad: PadResult) -> dict[str, Any]:
    record: dict[str, Any] = {
        "apin": pad.apin,
        "dp": pad.dp,
        "method": pad.method,
        "result": pad.result.value,
        "role": pad.role.value,
    }
    # Measurement groups that weren't taken are left out, never zero-filled.
    for key in _MEASUREMENT_KEYS:
        value = getattr(pad, key)
        if value is not None:
            record[key] = value
    return record


def _context_record(ctx: TestContext) -> dict[str, Any]:
    record: dict[str, Any] = {
        "aid": ctx.aid,
        "ahw": ctx.ahw,
        "ins": ctx.ins,
        "tests": ctx.tests,
        "pm": ctx.pm,
        "currentListUa": ctx.current_list_ua,
        "testerUid": ctx.tester_uid,
        "testerBuild": ctx.tester_build,
    }
    if ctx.max_bond_r_ohms is not None:
        record["maxBondROhms"] = ctx.max_bond_r_ohms
    if ctx.cap_time_list_us is not None:
        record["capTimeListUs"] = ctx.cap_time_list_us
    if ctx.pm_name:
        record["pmName"] = ctx.pm_name
    return record


# ——— reading ————————————————————————————————————————————————————————————————

def from_record(record: dict[str, Any]) -> TestResult:
    """Rebuild a TestResult from a JSON object. Raises KeyError/ValueError/TypeError
    if a required field is missing or has the wrong type."""
    ctx = record.get("context")
    return TestResult(
        ts=datetime.fromisoformat(record["ts"]),
        outcome=record["outcome"],
        good=int(record["good"]),
        tested=int(record["tested"]),
        reason=record.get("reason"),
        slot=int(record.get("slot", 0)),
        pads=[_pad_from(p) for p in record.get("pads", [])],
        context=_context_from(ctx) if ctx else None,
        panel_id=record.get("panel_id"),
        img=record.get("img"),
        thumb=record.get("thumb"),
    )


def load(path: Path) -> list[TestResult]:
    """All readable results in a JSONL file, in file order.

    Lines that aren't valid JSON or lack required fields are skipped, so one
    damaged line (e.g. from a crash mid-write) doesn't hide the rest.
    """
    results = []
    with path.open(encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            try:
                results.append(from_record(json.loads(line)))
            except (ValueError, KeyError, TypeError):
                continue
    return results


def _pad_from(record: dict[str, Any]) -> PadResult:
    role = record.get("role", "IO")
    return PadResult(
        apin=int(record["apin"]),
        dp=int(record["dp"]),
        method=record.get("method", "STD"),
        result=BondResult(record["result"]),
        role=PadRole(role) if role in PadRole.__members__ else PadRole.IO,
        **{key: record.get(key) for key in _MEASUREMENT_KEYS},
    )


def _context_from(record: dict[str, Any]) -> TestContext:
    return TestContext(
        aid=record.get("aid", ""),
        ahw=int(record.get("ahw", 0)),
        ins=int(record.get("ins", 0)),
        tests=int(record.get("tests", 0)),
        pm=int(record.get("pm", 0)),
        pm_name=record.get("pmName", ""),
        current_list_ua=record.get("currentListUa", []),
        max_bond_r_ohms=record.get("maxBondROhms"),
        cap_time_list_us=record.get("capTimeListUs"),
        tester_uid=record.get("testerUid", ""),
        tester_build=record.get("testerBuild", ""),
    )

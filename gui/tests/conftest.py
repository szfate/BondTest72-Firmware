from pathlib import Path

import pytest

DATA = Path(__file__).parent / "data"

# Root of the firmware repository this package lives in (gui/ is one level down).
REPO_ROOT = Path(__file__).resolve().parents[2]


@pytest.fixture
def trace_lines() -> list[str]:
    """A real capture from a tester: HELLO, LIST_PADMAPS, then one full test of
    pad map 2 on an empty substrate (every pad OPEN), with the firmware's log
    lines interleaved. Serial numbers are anonymised."""
    return (DATA / "trace_pm2_empty_substrate.txt").read_text().splitlines()
